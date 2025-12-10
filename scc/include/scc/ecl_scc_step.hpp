#pragma once

#include <memory/bit_accessor.hpp>
#include <scc/base.hpp>
#include <scc/edge_frontier.hpp>
#include <scc/graph.hpp>
#include <scc/part.hpp>

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
  vertex_t*   ecl_fw_lable,
  vertex_t*   ecl_bw_lable)
{
  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u    = part.to_global(k);
    ecl_fw_lable[k] = u;
    ecl_bw_lable[k] = u;
  }
}

template<WorldPartConcept Part>
void
ecl_scc_step(
  Part const&     part,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  vertex_t*       scc_id,
  vertex_t*       ecl_fw_lable,
  vertex_t*       ecl_bw_lable,
  BitAccessor     changed,
  edge_frontier&  frontier)
{
  // this function uses the project convention that:
  // k,l always describe local vertex ids
  // u,v always describe global vertex ids
  // (csr stores global vertex ids)
  // (head is accessed by local vertex ids)

  auto const local_n = part.local_n();

  auto const saturate_direction = [local_n, scc_id, &changed, &part, &frontier](index_t const* head, vertex_t const* csr, index_t const* opposite_head, vertex_t const* opposite_csr, vertex_t* ecl_lable) {
    changed.clear(local_n); // clean bit vector

    constexpr auto converged           = static_cast<u8>(0);
    constexpr auto not_converged       = static_cast<u8>(1);
    constexpr auto conversion_operator = MPI_LOR;

    u8 convergence_state;
    do { // while (convergence_state == not_converged)

      // === 0. ASSUME CONVERGED (update on change) ===
      convergence_state = converged;

      // === 1. LOCAL SATUATION ===

      auto local_converged = false;
      while (not local_converged) {
        local_converged = true;

        for (vertex_t k = 0; k < local_n; ++k) {
          if (scc_id[k] == scc_id_undecided) {
            auto const label_k   = ecl_lable[k];
            auto       min_label = label_k;

            // locally identify minimum in neigbourhood
            for (auto const v : csr_range(opposite_head, opposite_csr, k)) {
              if (part.has_local(v)) {
                auto const l = part.to_local(v);
                if (scc_id[l] == scc_id_undecided) {
                  min_label = std::min(min_label, ecl_lable[l]);
                }
              }
            }

            if (min_label != label_k) {
              ecl_lable[k]    = min_label;
              local_converged = false;
              changed.set(k);
            }
          }
        }
      }

      // === 2. COMMUNICATE BORDERS ===

      for (vertex_t k = 0; k < local_n; ++k) {
        if (changed.get(k)) { // if k changed then k must be undecided
          for (auto const v : csr_range(head, csr, k)) {
            if (not part.has_local(v)) { // uv non local -> propagate minimum to v (via comm)
              DEBUG_ASSERT_NE(part.world_rank_of(v), mpi_world_rank, "part.has_local and part.world_rank_of are not consistent");
              frontier.push(part.world_rank_of(v), { v, ecl_lable[k] });
            }
          }
        }
      }

      // any comm is set the same accross all ranks and is only false if no rank has send any message
      auto const any_communication = frontier.comm(part);

      // === 3. PROCESS NON LOCAL CHANGES ===

      changed.clear(local_n); // clear for next step (notice: will also ensure clean bitvector when lambda finishes)

      if (any_communication) {
        do {
          auto const [u, label] = frontier.next();
          DEBUG_ASSERT(part.has_local(u), "should never receive vertices that are not local");
          auto const k = part.to_local(u);
          if (scc_id[k] == scc_id_undecided) {
            if (label < ecl_lable[k]) {
              ecl_lable[k]      = label;
              convergence_state = not_converged;
              changed.set(k);
            }
          }
        } while (frontier.has_next());
        convergence_state = mpi_basic_allreduce_single(convergence_state, conversion_operator);
      }
    } while (convergence_state == not_converged);
  };

  saturate_direction(fw_head, fw_csr, bw_head, bw_csr, ecl_fw_lable);
  saturate_direction(bw_head, bw_csr, fw_head, fw_csr, ecl_bw_lable);

  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      if (ecl_fw_lable[k] == ecl_bw_lable[k]) {
        scc_id[k] = ecl_fw_lable[k];
      }
    }
  }
}
