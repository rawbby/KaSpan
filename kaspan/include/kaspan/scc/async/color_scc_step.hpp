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
  u64*                       active_storage) -> vertex_t
{
  auto       part      = graph.part;
  auto const local_n   = part.local_n();
  auto       is_active = view_bits(active_storage, local_n);

  DEBUG_ASSERT(active.empty());

  // Phase 1: Forward Color Propagation
  {
    auto on_message = [&](auto env) {
      for (auto const& edge : env.message) {
        auto const u     = edge.u;
        auto const label = edge.v;
        DEBUG_ASSERT(part.has_local(u));
        auto const k = part.to_local(u);
        if (scc_id[k] == scc_id_undecided && label < colors[k]) {
          colors[k] = label;
          if (!is_active.get(k)) {
            is_active.set(k);
            active.push_back(k);
          }
        }
      }
    };

    is_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    is_active.for_each(local_n, [&](auto&& k) { active.push_back(k); });

    mq.reactivate();

    do {
      while (!active.empty()) {
        auto const k = active.back();
        active.pop_back();
        is_active.unset(k);

        auto const label = colors[k];
        for (auto v : graph.csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided && label < colors[l]) {
              colors[l] = label;
              if (!is_active.get(l)) {
                is_active.set(l);
                active.push_back(l);
              }
            }
          } else {
            mq.post_message_blocking(edge_t{ v, label }, part.world_rank_of(v), on_message);
          }
        }
        mq.poll_throttled(on_message);
      }
    } while (!mq.terminate(on_message));
  }

  DEBUG_ASSERT(active.empty());

  // Phase 2: Multi-pivot Backward Search
  vertex_t local_decided_count = 0;
  {
    auto on_message = [&](auto env) {
      for (auto const& edge : env.message) {
        auto const u     = edge.u;
        auto const pivot = edge.v;
        DEBUG_ASSERT(part.has_local(u));
        auto const k = part.to_local(u);
        if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
          scc_id[k] = pivot;
          ++local_decided_count;
          if (!is_active.get(k)) {
            is_active.set(k);
            active.push_back(k);
          }
        }
      }
    };

    DEBUG_ASSERT(active.empty());
    is_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    is_active.for_each(local_n, [&](auto k) {
      if (colors[k] == part.to_global(k)) {
        scc_id[k] = colors[k];
        ++local_decided_count;
        active.push_back(k);
      } else {
        is_active.unset(k);
      }
    });

    mq.reactivate();

    do {
      while (!active.empty()) {
        auto const k = active.back();
        active.pop_back();
        is_active.unset(k);

        auto const pivot = colors[k];
        for (auto v : graph.bw_csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided && colors[l] == pivot) {
              scc_id[l] = pivot;
              ++local_decided_count;
              if (!is_active.get(l)) {
                is_active.set(l);
                active.push_back(l);
              }
            }
          } else {
            mq.post_message_blocking(edge_t{ v, pivot }, part.world_rank_of(v), on_message);
          }
        }
        mq.poll_throttled(on_message);
      }
    } while (!mq.terminate(on_message));
  }

  DEBUG_ASSERT(active.empty());

  return local_decided_count;
}

} // namespace kaspan::async
