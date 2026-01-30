#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

namespace kaspan {

template<u64               Threshold,
         part_view_concept Part>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  frontier_view<edge_t,
                Threshold>   message_queue,
  vector<vertex_t>&          message_buffer,
  vertex_t*                  colors,
  vertex_t*                  scc_id,
  auto&&                     on_decision) -> vertex_t
{
  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    colors[k] = graph.part.to_global(k);
  }

  // Phase 1: Forward Color Propagation
  auto on_message = [&](edge_t e) {
    auto const k     = graph.part.to_local(e.u);
    auto const label = e.v;
    if (scc_id[k] == scc_id_undecided && label < colors[k]) {
      colors[k] = label;
      message_buffer.push_back(k);
    }
  };

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      message_buffer.push_back(k);
    }
  }

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
          message_queue.push(graph.part, graph.part.world_rank_of(v), { v, label }, on_message);
        }
      }
    }
  } while (message_queue.comm(graph.part, on_message));

  // Phase 2: Multi-pivot Backward Search
  vertex_t local_decided_count = 0;

  auto bw_on_message = [&](edge_t e) {
    auto const k     = graph.part.to_local(e.u);
    auto const pivot = e.v;
    if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
      scc_id[k] = pivot;
      on_decision(k);
      ++local_decided_count;
      message_buffer.push_back(k);
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
          message_queue.push(graph.part, graph.part.world_rank_of(v), { v, pivot }, bw_on_message);
        }
      }
    }
  } while (message_queue.comm(graph.part, bw_on_message));

  return local_decided_count;
}

template<u64               Threshold,
         part_view_concept Part>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  frontier_view<edge_t,
                Threshold>   message_queue,
  vector<vertex_t>&          message_buffer,
  vertex_t*                  colors,
  vertex_t*                  scc_id) -> vertex_t
{
  auto const on_message = [](auto /* v */) {};
  return color_scc_step(graph, message_queue, message_buffer, colors, scc_id, on_message);
}

} // namespace kaspan
