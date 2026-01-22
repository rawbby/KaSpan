#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <vector>

namespace kaspan {

template<part_view_concept Part>
auto
backward_search(
  bidi_graph_part_view<Part> graph,
  frontier_view<vertex_t>    frontier,
  vertex_t*                  scc_id,
  u64*                       fw_reached_storage,
  u64*                       dense_front_storage,
  vertex_t                   pivot,
  vertex_t                   local_decided,
  auto&&                     on_decision = [](vertex_t) {}) -> vertex_t
{
  vertex_t decided_count = 0;

  auto fw_reached     = view_bits(fw_reached_storage, graph.part.local_n());
  auto dense_frontier = view_bits(dense_front_storage, graph.part.local_n());

  if (graph.part.has_local(pivot)) frontier.local_push(pivot);

  auto const dense_threshold = std::max<vertex_t>(1, graph.part.local_n() / 64);

  do {
    while (frontier.size() >= dense_threshold) {
      while (frontier.has_next()) {
        dense_frontier.set(graph.part.to_local(frontier.next()));
      }
      dense_frontier.for_each(graph.part.local_n(), [&](auto k) {
        if (fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
          scc_id[k] = pivot;
          on_decision(k);
          ++decided_count;
          graph.each_bw_v(k, [&](auto v) { frontier.relaxed_push(graph.part, v); });
        }
      });
      dense_frontier.clear(graph.part.local_n());
    }

    while (frontier.has_next()) {
      auto const k = graph.part.to_local(frontier.next());
      if (fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
        scc_id[k] = pivot;
        on_decision(k);
        ++decided_count;
        graph.each_bw_v(k, [&](auto v) { frontier.relaxed_push(graph.part, v); });
      }
    }
  } while (frontier.comm(graph.part));

  return decided_count;
}

} // namespace kaspan
