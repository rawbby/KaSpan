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
  BitsAccessor     fw_reached,
  vertex_t         root) -> vertex_t
{
  vertex_t local_min = part.n; // Initialize to max possible value
  bool processed_any = false;
  
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
      processed_any = true;

      // now it is reached - update minimum ONLY for undecided vertices
      fw_reached.set(k);
      local_min = std::min(local_min, u);
      
      DEBUG_ASSERT_EQ(scc_id[k], scc_id_undecided, 
        "BUG: Processing vertex that is already decided! k={}, u={}, scc_id[k]={}", 
        k, u, scc_id[k]);

      // add all neighbours to frontier
      for (vertex_t v : csr_range(fw_head, fw_csr, k)) {
        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
  } while (frontier.comm(part));

  auto const global_min = mpi_basic_allreduce_single(local_min, MPI_MIN);
  
  DEBUG_ASSERT_LT(global_min, part.n, 
    "BUG in forward_search: global_min={} should be < n={}. "
    "This means no rank processed any vertices, which should be impossible if root is valid.",
    global_min, part.n);
  
  return global_min;
}
