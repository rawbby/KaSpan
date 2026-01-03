#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/vertex_frontier.hpp>

namespace kaspan {

template<world_part_concept part_t>
auto
backward_search(part_t const& part, index_t const* bw_head, vertex_t const* bw_csr, vertex_frontier& frontier, vertex_t* scc_id, bits_accessor fw_reached, vertex_t pivot)
  -> vertex_t
{
  auto const local_n = part.local_n();

  vertex_t decided_count = 0;
  vertex_t min_u         = part.n;

  if (part.has_local(pivot)) { frontier.local_push(pivot); }

  do {
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(part.has_local(u));
      auto const k = part.to_local(u);

      if (!fw_reached.get(k) or scc_id[k] != scc_id_undecided) { continue; }

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = pivot;
      min_u     = std::min(min_u, u);
      ++decided_count;

      // add all neighbours to frontier
      for (vertex_t const v : csr_range(bw_head, bw_csr, k)) {
        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
  } while (frontier.comm(part));

  // normalise scc_id to minimum vertex in scc
  min_u = mpi_basic::allreduce_single(min_u, mpi_basic::min);
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == pivot) { scc_id[k] = min_u; }
  }

  return decided_count;
}

} // namespace kaspan
