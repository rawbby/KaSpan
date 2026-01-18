#pragma once

#include <kaspan/graph/graph_part.hpp>
#include <kaspan/scc/base.hpp>

namespace kaspan {

template<bool InterleavedSupport = false, part_concept Part>
void
forward_search(
  graph_part_view<Part>                graph,
  vertex_frontier<InterleavedSupport>& frontier,
  vertex_t const*                      scc_id,
  u64*                                 fw_reached_storage,
  vertex_t                             pivot)
{
  auto const& part          = *graph.part;
  auto const  local_n       = part.local_n();
  auto        fw_reached    = view_bits(fw_reached_storage, local_n);

  DEBUG_ASSERT_NOT(frontier.has_next());
  if (part.has_local(pivot)) {
    DEBUG_ASSERT(scc_id[part.to_local(pivot)] == scc_id_undecided, "pivot={}", pivot);
    frontier.local_push(pivot);
  }

  do {
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(part.has_local(u));
      auto const k = part.to_local(u);

      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided) {
        continue;
      }

      fw_reached.set(k);

      for (vertex_t const v : graph.csr_range(k)) {
        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
  } while (frontier.comm(part));
}

} // namespace kaspan
