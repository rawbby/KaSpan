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
  brief_queue_t&             mq,
  vertex_t*                  scc_id,
  vertex_t*                  colors,
  vector<vertex_t>&          active,
  auto&&                     on_decision) -> vertex_t
{
  auto       part    = graph.part;
  auto const local_n = part.local_n();

  DEBUG_ASSERT(active.empty());

  for (vertex_t k = 0; k < local_n; ++k) {
    colors[k] = part.to_global(k);
  }

  // Phase 1: Forward Color Propagation
  {
    auto on_messages = [&](auto env) {
      for (auto const& edge : env.message) {
        auto const k     = part.to_local(edge.u);
        auto const label = edge.v;
        if (scc_id[k] == scc_id_undecided && label < colors[k]) {
          colors[k] = label;
          active.push_back(k);
        }
      }
    };

    for (vertex_t k = 0; k < local_n; ++k) {
      if (scc_id[k] == scc_id_undecided) {
        active.push_back(k);
      }
    }

    mq.reactivate();

    do {
      while (!active.empty()) {
        auto const k = active.back();
        active.pop_back();

        auto const label = colors[k];
        for (auto v : graph.csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided && label < colors[l]) {
              colors[l] = label;
              active.push_back(l);
            }
          } else if (label < v) {
            mq.post_message_blocking(edge_t{ v, label }, part.world_rank_of(v), on_messages);
          }
        }
        mq.poll_throttled(on_messages);
      }
    } while (!mq.terminate(on_messages));
  }

  DEBUG_ASSERT(active.empty());

  // Phase 2: Multi-pivot Backward Search
  vertex_t local_decided_count = 0;
  {
    auto on_messages = [&](auto env) {
      for (auto const& edge : env.message) {
        auto const k     = part.to_local(edge.u);
        auto const pivot = edge.v;
        if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
          scc_id[k] = pivot;
          on_decision(k);
          ++local_decided_count;
          active.push_back(k);
        }
      }
    };

    for (vertex_t k = 0; k < local_n; ++k) {
      if (scc_id[k] == scc_id_undecided && colors[k] == part.to_global(k)) {
        scc_id[k] = colors[k];
        on_decision(k);
        ++local_decided_count;
        active.push_back(k);
      }
    }

    mq.reactivate();

    do {
      while (!active.empty()) {
        auto const k = active.back();
        active.pop_back();

        auto const pivot = colors[k];
        for (auto v : graph.bw_csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided && colors[l] == pivot) {
              scc_id[l] = pivot;
              on_decision(l);
              ++local_decided_count;
              active.push_back(l);
            }
          } else {
            mq.post_message_blocking(edge_t{ v, pivot }, part.world_rank_of(v), on_messages);
          }
        }
        mq.poll_throttled(on_messages);
      }
    } while (!mq.terminate(on_messages));
  }

  DEBUG_ASSERT(active.empty());

  return local_decided_count;
}

template<part_view_concept Part,
         typename brief_queue_t>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  brief_queue_t&             mq,
  vertex_t*                  scc_id,
  vertex_t*                  colors,
  vector<vertex_t>&          active) -> vertex_t
{
  auto const on_decision = [](auto /* v */) {};
  return color_scc_step(graph, mq, scc_id, colors, active, on_decision);
}

} // namespace kaspan::async
