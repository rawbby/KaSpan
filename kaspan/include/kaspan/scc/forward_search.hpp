#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/scc/frontier.hpp>

namespace kaspan {

template<part_view_concept Part>
void
forward_search(
  bidi_graph_part_view<Part> graph,
  frontier_view<vertex_t>    frontier,
  vertex_t const*            scc_id,
  u64*                       fw_reached_storage,
  vertex_t                   pivot,
  vertex_t                   local_decided)
{
  auto fw_reached = view_bits(fw_reached_storage, graph.part.local_n());

  if (graph.part.has_local(pivot)) {
    frontier.local_push(pivot);
  }

  do {
    while (frontier.has_next()) {
      auto const k = graph.part.to_local(frontier.next());

      if (!fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
        fw_reached.set(k);
        graph.each_v(k, [&](auto v) { frontier.relaxed_push(graph.part, v); });
      }
    }
  } while (frontier.comm(graph.part));
}

} // namespace kaspan
