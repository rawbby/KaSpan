#pragma once

#include <memory/bit_accessor.hpp>
#include <scc/base.hpp>
#include <scc/edge_frontier.hpp>
#include <scc/graph.hpp>
#include <scc/part.hpp>

#include <functional>

/* The ECL label propagation partitions the graph into compontents that are gurateed to be supersets of strongly connected components.
 * Further per weakly connected component the ECL label propagation identifies one strongly connected component per weakly connected component.
 *
 * Idea of the algorithm: extremely parallel forward backward search via fw and bw label pair.
 * min label is propageted along fw and and bw graph. resulting in one scc found per wcc with root as min vertex in wcc.
 * The result can be used for further iteration as each label pair identifies a superset of scc components.
 * (scc subset ecl-component subset wcc subset graph)
 * if fw_label and bw_label are equal, ecl-componen is scc component.
 */

template<WorldPartConcept Part>
void
ecl_scc_init_lable(
  Part const& part,
  vertex_t*   ecl_fw_label,
  vertex_t*   ecl_bw_label)
{
  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u    = part.to_global(k);
    ecl_fw_label[k] = u;
    ecl_bw_label[k] = u;
  }
}

template<WorldPartConcept Part>
auto
ecl_scc_step(
  Part const&     part,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  vertex_t*       scc_id,

  vertex_t* ecl_fw_label,
  vertex_t* ecl_bw_label,

  StackAccessor<vertex_t> active_stack,
  BitAccessor             active,

  // StackAccessor<vertex_t> changed_stack,
  BitAccessor             changed,

  edge_frontier& frontier) -> vertex_t
{
  // this function uses the project convention that:
  // k,l always describe local vertex ids
  // u,v always describe global vertex ids
  // (csr stores global vertex ids)
  // (head is accessed by local vertex ids)

  auto const local_n = part.local_n();

  auto const saturate_direction = [&](index_t const* head, vertex_t const* csr, index_t const* opposite_head, vertex_t const* opposite_csr, vertex_t* ecl_lable) {
    active.fill_cmp(local_n, scc_id, scc_id_undecided);
    changed.fill_cmp(local_n, scc_id, scc_id_undecided);
    active.for_each(local_n, [&](auto&& k) {
      active_stack.push(k);
      // changed_stack.push(k);
    });

    for (;;) {
      // local satuation
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();

        auto const u         = part.to_global(k);
        auto const label_k   = ecl_lable[k];
        auto       min_label = label_k;

        for (auto&& v : csr_range(opposite_head, opposite_csr, k)) {
          if (part.has_local(v)) {
            auto const l       = part.to_local(v);
            auto const label_l = ecl_lable[l];
            min_label          = std::min(min_label, label_l);
          }
        }

        if (min_label < label_k) {
          ecl_lable[k] = min_label;
          if (not changed.get(k)) {
            changed.set(k);
            // changed_stack.push(k);
          }
          for (auto&& v : csr_range(head, csr, k)) {
            if (part.has_local(v)) {
              auto const l = part.to_local(v);
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }

        active.unset(k);
      }

      // communicate changes to neighbours
      // while (not changed_stack.empty()) {
      //   auto const k = changed_stack.back();
      //   changed_stack.pop();
      changed.for_each(local_n, [&] (auto&&k) {
        auto const label_k = ecl_lable[k];
        for (auto&& v : csr_range(head, csr, k)) {
          if (not part.has_local(v)) {
            frontier.push(part.world_rank_of(v), { v, label_k });
          }
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, label] = frontier.next();
        auto const k          = part.to_local(u);

        if (label < ecl_lable[k]) {
          ecl_lable[k] = label;
          if (not changed.get(k)) { // must be communicated next iteration
            changed.set(k);
            // changed_stack.push(k);
          }
        }
        for (auto&& v : csr_range(head, csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (not active.get(l)) {
              active.set(l);
              active_stack.push(l);
            }
          }
        }
      }
    }
  };

  saturate_direction(fw_head, fw_csr, bw_head, bw_csr, ecl_fw_label);
  saturate_direction(bw_head, bw_csr, fw_head, fw_csr, ecl_bw_label);

  vertex_t local_component_count = 0;
  vertex_t local_decided_count   = 0;

  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const label = ecl_fw_label[k];
      if (label == ecl_bw_label[k]) {
        scc_id[k] = label;
        ++local_decided_count;
        if (part.to_global(k) == label) {
          // this component is counted to this rank
          ++local_component_count;
        }
      }
    }
  }

  DEBUG_ASSERT_GT(mpi_basic_allreduce_single(local_decided_count, MPI_SUM), 0);
  return local_decided_count;
}
