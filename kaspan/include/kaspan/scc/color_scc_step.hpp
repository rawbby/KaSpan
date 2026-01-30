#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
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
  u64*                       in_message_storage,
  u64*                       is_changed_storage,
  vertex_t*                  colors,
  vertex_t*                  scc_id,
  auto&&                     on_decision) -> vertex_t
{
  auto in_message = view_bits(in_message_storage, graph.part.local_n());
  auto is_changed = view_bits(is_changed_storage, graph.part.local_n());

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    colors[k] = graph.part.to_global(k);
  }

  // Phase 1: Forward Color Propagation (inspired by HPCGraph scc_color)
  // This partitions the graph into components that are supersets of SCCs.
  // Each vertex v gets the minimum ID of a vertex u that can reach it (u -> v).
  {
    in_message.set_each(graph.part.local_n(), SCC_ID_UNDECIDED_FILTER(graph.part.local_n(), scc_id));
    std::memcpy(is_changed.data(), in_message.data(), (graph.part.local_n() + 7) >> 3);
    in_message.for_each(graph.part.local_n(), [&](auto k) { message_buffer.push_back(k); });

    while (true) {
      while (!message_buffer.empty()) {
        auto const k = message_buffer.back();
        message_buffer.pop_back();

        auto const label = colors[k];
        for (auto v : graph.csr_range(k)) {
          if (graph.part.has_local(v)) {
            auto const l = graph.part.to_local(v);
            if (scc_id[l] == scc_id_undecided && label < colors[l]) {
              colors[l] = label;
              is_changed.set(l);
              if (!in_message.get(l)) {
                in_message.set(l);
                message_buffer.push_back(l);
              }
            }
          }
        }
        in_message.unset(k);
      }

      is_changed.for_each(graph.part.local_n(), [&](auto k) {
        auto const label_k = colors[k];
        for (auto v : graph.csr_range(k)) {
          if (label_k < v && !graph.part.has_local(v)) {
            message_queue.push(graph.part, graph.part.world_rank_of(v), { v, label_k });
          }
        }
      });
      is_changed.clear(graph.part.local_n());

      if (!message_queue.comm(graph.part)) {
        break;
      }

      while (message_queue.has_next()) {
        auto const [u, label] = message_queue.next();
        auto const k          = graph.part.to_local(u);
        if (scc_id[k] == scc_id_undecided && label < colors[k]) {
          colors[k] = label;
          is_changed.set(k);
          if (!in_message.get(k)) {
            in_message.set(k);
            message_buffer.push_back(k);
          }
        }
      }
    }
  }

  // Phase 2: Multi-pivot Backward Search (inspired by HPCGraph scc_find_sccs)
  // For each color/component, the vertex u where colors[u] == u is the pivot.
  // We perform a backward search from these pivots, restricted to the same color.
  // Intersection of forward reaching (Phase 1) and backward reaching (Phase 2) is the SCC.
  vertex_t local_decided_count = 0;
  {
    DEBUG_ASSERT(message_buffer.empty());
    in_message.set_each(graph.part.local_n(), SCC_ID_UNDECIDED_FILTER(graph.part.local_n(), scc_id));
    is_changed.clear(graph.part.local_n());
    in_message.for_each(graph.part.local_n(), [&](auto k) {
      if (colors[k] == graph.part.to_global(k)) {
        scc_id[k] = colors[k];
        on_decision(k);
        ++local_decided_count;
        is_changed.set(k);
        message_buffer.push_back(k);
      } else {
        in_message.unset(k);
      }
    });

    while (true) {
      while (!message_buffer.empty()) {
        auto const k = message_buffer.back();
        message_buffer.pop_back();
        in_message.unset(k);

        auto const pivot = colors[k];
        for (auto v : graph.bw_csr_range(k)) {
          if (graph.part.has_local(v)) {
            auto const l = graph.part.to_local(v);
            if (scc_id[l] == scc_id_undecided && colors[l] == pivot) {
              scc_id[l] = pivot;
              on_decision(l);
              ++local_decided_count;
              is_changed.set(l);
              if (!in_message.get(l)) {
                in_message.set(l);
                message_buffer.push_back(l);
              }
            }
          }
        }
      }

      is_changed.for_each(graph.part.local_n(), [&](auto k) {
        auto const pivot = colors[k];
        for (auto v : graph.bw_csr_range(k)) {
          if (!graph.part.has_local(v)) {
            message_queue.push(graph.part, graph.part.world_rank_of(v), { v, pivot });
          }
        }
      });
      is_changed.clear(graph.part.local_n());

      if (!message_queue.comm(graph.part)) {
        break;
      }

      while (message_queue.has_next()) {
        auto const [u, pivot] = message_queue.next();
        auto const k          = graph.part.to_local(u);
        if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
          scc_id[k] = pivot;
          on_decision(k);
          ++local_decided_count;
          is_changed.set(k);
          if (!in_message.get(k)) {
            in_message.set(k);
            message_buffer.push_back(k);
          }
        }
      }
    }
  }

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
  u64*                       in_message_storage,
  u64*                       is_changed_storage,
  vertex_t*                  colors,
  vertex_t*                  scc_id) -> vertex_t
{
  return color_scc_step(graph, message_queue, message_buffer, in_message_storage, is_changed_storage, colors, scc_id, [](auto /* v */) {});
}

} // namespace kaspan
