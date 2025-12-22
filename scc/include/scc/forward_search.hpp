#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <scc/base.hpp>
#include <util/pp.hpp>

template<WorldPartConcept Part>
auto
forward_search(
  Part const&      part,
  index_t const*   fw_head,
  vertex_t const*  fw_csr,
  vertex_frontier& frontier,
  vertex_t const*  scc_id,
  BitsAccessor      fw_reached,
  vertex_t         root) -> vertex_t
{
  if (part.has_local(root)) {
    frontier.local_push(root);
    ASSERT(not fw_reached.get(part.to_local(root)));
    ASSERT(scc_id[part.to_local(root)] == scc_id_undecided, "root=%d", root);
  }

  do {
    size_t processed_count = 0;

    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = part.to_local(u);

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      ++processed_count;

      // now it is reached
      fw_reached.set(k);
      root = std::min(root, u);

      // add all neighbours to frontier
      for (vertex_t v : csr_range(fw_head, fw_csr, k)) {
        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
  } while(frontier.comm(part));

  return mpi_basic_allreduce_single(root, MPI_MIN);
}
