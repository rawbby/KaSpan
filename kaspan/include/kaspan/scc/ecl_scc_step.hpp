#pragma once

#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/edge_frontier.hpp>
#include <kaspan/scc/graph.hpp>
#include <kaspan/scc/part.hpp>

#include <functional>

namespace kaspan {

/* The ECL label propagation partitions the graph into compontents that are gurateed to be supersets of strongly connected components.
 * Further per weakly connected component the ECL label propagation identifies one strongly connected component per weakly connected component.
 *
 * Idea of the algorithm: extremely parallel forward backward search via fw and bw label pair.
 * min label is propageted along fw and and bw graph. resulting in one scc found per wcc with root as min vertex in wcc.
 * The result can be used for further iteration as each label pair identifies a superset of scc components.
 * (scc subset ecl-component subset wcc subset graph)
 * if fw_label and bw_label are equal, ecl-componen is scc component.
 */

template<world_part_concept part_t>
void
ecl_scc_init_lable(part_t const& part, vertex_t* ecl_fw_label, vertex_t* ecl_bw_label)
{
  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u    = part.to_global(k);
    ecl_fw_label[k] = u;
    ecl_bw_label[k] = u;
  }
}

template<world_part_concept part_t>
auto
ecl_scc_step(part_t const&   part,
             index_t const*  fw_head,
             vertex_t const* fw_csr,
             index_t const*  bw_head,
             vertex_t const* bw_csr,
             vertex_t*       scc_id,
             vertex_t*       ecl_fw_label,
             vertex_t*       ecl_bw_label,
             vertex_t*       active_array,
             u64*            active_storage,
             u64*            changed_storage,
             edge_frontier&  frontier,
             vertex_t        decided_count = 0) -> vertex_t
{
  // this function uses the project convention that:
  // k,l always describe local vertex ids
  // u,v always describe global vertex ids
  // (csr stores global vertex ids)
  // (head is accessed by local vertex ids)

  auto const local_n      = part.local_n();
  auto       active       = view_bits(active_storage, local_n);
  auto       changed      = view_bits(changed_storage, local_n);
  auto       active_stack = view_stack<vertex_t>(active_array, local_n - decided_count);

#if KASPAN_DEBUG
  // Validate decided_count is consistent with scc_id
  vertex_t actual_decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] != scc_id_undecided) {
      ++actual_decided_count;
    }
  }
  DEBUG_ASSERT_EQ(actual_decided_count, decided_count);
#endif

  auto const saturate_direction = [&](index_t const* head, vertex_t const* csr, vertex_t* ecl_lable) {
    DEBUG_ASSERT(active_stack.empty());
    DEBUG_ASSERT(not frontier.has_next());

    active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    std::memcpy(changed.data(), active.data(), (local_n + 7) >> 3);
    active.for_each(local_n, [&](auto&& k) {
      active_stack.push(k);
    });

    while (true) {

      // === LOCAL SATUATION ===

      // this step iterates local until satuation
      // to reduce communication overhead.

      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();

        auto const label = ecl_lable[k];
        for (auto v : csr_range(head, csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (label < ecl_lable[l] and scc_id[l] == scc_id_undecided) {
              ecl_lable[l] = label;
              changed.set(l);
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }

        active.unset(k);
      }

      DEBUG_ASSERT(active_stack.empty());

      // === COMMUNICATE (CHANGED) LABELS ===

      changed.for_each(local_n, [&](auto&& k) {
        auto const label_k = ecl_lable[k];
        for (auto v : csr_range(head, csr, k)) {
          if (label_k < v and not part.has_local(v)) {
            DEBUG_ASSERT_NE(part.world_rank_of(v), mpi_basic::world_rank);
            frontier.push(part.world_rank_of(v), { v, label_k });
          }
        }
      });
      changed.clear(local_n);

      // === CHECK CONVERGENCE ===

      // if no messages to exchange (globally)
      // the labels converged.

      if (not frontier.comm(part)) {
        break;
      }

      // === CHECK INCOMING PROPAGATIONS ===

      while (frontier.has_next()) {
        auto const [u, label] = frontier.next();
        DEBUG_ASSERT(part.has_local(u));
        auto const k = part.to_local(u);

        if (label < ecl_lable[k] and scc_id[k] == scc_id_undecided) {
          ecl_lable[k] = label;
          changed.set(k);
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
          }
        }
      }
    }
  };

  saturate_direction(fw_head, fw_csr, ecl_fw_label);
  saturate_direction(bw_head, bw_csr, ecl_bw_label);

  vertex_t local_decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const label = ecl_fw_label[k];
      if (label == ecl_bw_label[k]) {
        scc_id[k] = label;
        ++local_decided_count;
      }
    }
  }

  DEBUG_ASSERT_GT(mpi_basic::allreduce_single(local_decided_count, mpi_basic::sum), 0);
  return local_decided_count;
}

} // namespace kaspan
