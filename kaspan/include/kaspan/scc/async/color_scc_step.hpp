#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/vector.hpp>

namespace kaspan::async {

template<part_view_concept Part,
         typename brief_queue_t>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  brief_queue_t&             message_queue,
  vector<vertex_t>&          message_buffer,
  vertex_t*                  colors,
  vertex_t*                  scc_id,
  auto&&                     on_decision) -> vertex_t
{
  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    colors[k] = graph.part.to_global(k);
  }

  // Phase 1: Forward Color Propagation
  auto on_messages = [&](auto env) {
    for (auto const& edge : env.message) {
      auto const k     = graph.part.to_local(edge.u);
      auto const label = edge.v;
      if (scc_id[k] == scc_id_undecided && label < colors[k]) {
        colors[k] = label;
        message_buffer.push_back(k);
      }
    }
  };

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      message_buffer.push_back(k);
    }
  }

  message_queue.reactivate();

  do {
    while (!message_buffer.empty()) {
      auto const k = message_buffer.back();
      message_buffer.pop_back();

      auto const label = colors[k];
      for (auto v : graph.csr_range(k)) {
        if (graph.part.has_local(v)) {
          auto const l = graph.part.to_local(v);
          if (scc_id[l] == scc_id_undecided && label < colors[l]) {
            colors[l] = label;
            message_buffer.push_back(l);
          }
        } else if (label < v) {
          message_queue.post_message_blocking(edge_t{ v, label }, graph.part.world_rank_of(v), on_messages);
        }
      }
      message_queue.poll_throttled(on_messages);
    }
  } while (!message_queue.terminate(on_messages));

  // Phase 2: Multi-pivot Backward Search
  vertex_t local_decided_count = 0;

  auto bw_on_messages = [&](auto env) {
    for (auto const& edge : env.message) {
      auto const k     = graph.part.to_local(edge.u);
      auto const pivot = edge.v;
      if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
        scc_id[k] = pivot;
        on_decision(k);
        ++local_decided_count;
        message_buffer.push_back(k);
      }
    }
  };

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided && colors[k] == graph.part.to_global(k)) {
      scc_id[k] = colors[k];
      on_decision(k);
      ++local_decided_count;
      message_buffer.push_back(k);
    }
  }

  message_queue.reactivate();

  do {
    while (!message_buffer.empty()) {
      auto const k = message_buffer.back();
      message_buffer.pop_back();

      auto const pivot = colors[k];
      for (auto v : graph.bw_csr_range(k)) {
        if (graph.part.has_local(v)) {
          auto const l = graph.part.to_local(v);
          if (scc_id[l] == scc_id_undecided && colors[l] == pivot) {
            scc_id[l] = pivot;
            on_decision(l);
            ++local_decided_count;
            message_buffer.push_back(l);
          }
        } else if (pivot < v) {
          message_queue.post_message_blocking(edge_t{ v, pivot }, graph.part.world_rank_of(v), bw_on_messages);
        }
      }
      message_queue.poll_throttled(bw_on_messages);
    }
  } while (!message_queue.terminate(bw_on_messages));

  return local_decided_count;
}

template<part_view_concept Part,
         typename brief_queue_t>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  brief_queue_t&             message_queue,
  vector<vertex_t>&          message_buffer,
  vertex_t*                  colors,
  vertex_t*                  scc_id) -> vertex_t
{
  auto const on_decision = [](auto /* v */) {};
  return color_scc_step(graph, message_queue, message_buffer, colors, scc_id, on_decision);
}

} // namespace kaspan::async
