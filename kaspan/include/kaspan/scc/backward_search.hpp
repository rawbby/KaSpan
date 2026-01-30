#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/scc/frontier.hpp>

namespace kaspan {

template<u64               Threshold,
         part_view_concept Part>
auto
backward_search(
  bidi_graph_part_view<Part> graph,
  frontier_view<vertex_t,
                Threshold>   frontier,
  vector<vertex_t>&          message_buffer,
  vertex_t*                  scc_id,
  u64*                       fw_reached_storage,
  vertex_t                   root,
  auto&&                     on_decision) -> vertex_t
{
  auto     fw_reached    = view_bits(fw_reached_storage, graph.part.local_n());
  vertex_t decided_count = 0;

  DEBUG_ASSERT(message_buffer.empty());

  auto on_message = [&](auto v) {
    DEBUG_ASSERT(graph.part.has_local(v));
    auto const k = graph.part.to_local(v);
    if (fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
      scc_id[k] = root;
      ++decided_count;
      on_decision(k);
      message_buffer.push_back(k);
    }
  };

  if (graph.part.has_local(root)) {
    auto const k = graph.part.to_local(root);
    if (fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
      scc_id[k] = root;
      ++decided_count;
      on_decision(k);
      message_buffer.push_back(k);
    }
  }

  do {
    while (!message_buffer.empty()) {
      auto const k = message_buffer.back();
      message_buffer.pop_back();

      for (auto v : graph.bw_csr_range(k)) {
        if (graph.part.has_local(v)) {
          auto const l = graph.part.to_local(v);
          if (fw_reached.get(l) && scc_id[l] == scc_id_undecided) {
            scc_id[l] = root;
            ++decided_count;
            on_decision(l);
            message_buffer.push_back(l);
          }
        } else {
          frontier.push(graph.part, v, on_message);
        }
      }
    }
  } while (frontier.comm(graph.part, on_message));

  DEBUG_ASSERT(message_buffer.empty());

  return decided_count;
}

template<u64               Threshold,
         part_view_concept Part>
auto
backward_search(
  bidi_graph_part_view<Part> graph,
  frontier_view<vertex_t,
                Threshold>   frontier,
  vector<vertex_t>&          message_buffer,
  vertex_t*                  scc_id,
  u64*                       fw_reached_storage,
  vertex_t                   root) -> vertex_t
{
  auto const on_message = [](auto /* v */) {};
  return backward_search(graph, frontier, message_buffer, scc_id, fw_reached_storage, root, on_message);
}

} // namespace kaspan
