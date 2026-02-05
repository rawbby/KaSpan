#pragma once

#include <kaspan/memory/accessor/hash_map.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/math.hpp>

#include <cstring>

namespace kaspan {

template<part_view_c Part = single_part_view>
void
cache_forward_backward_search(
  bidi_graph_part_view<Part> g,
  frontier&                  front,
  vertex_t*                  active_storage,
  u64*                       is_reached_storage,
  u64*                       is_undecided_storage,
  vertex_t                   root,
  auto&&                     on_decision,
  hash_map<vertex_t>&        cache)
{
  auto active       = view_stack<vertex_t>(active_storage, g.part.local_n());
  auto is_reached   = view_bits_clean(is_reached_storage, g.part.local_n());
  auto is_undecided = view_bits(is_undecided_storage, g.part.local_n());
  auto fw_front     = front.view<edge_t>();
  auto bw_front     = front.view<edge_t>();

  // forward search

  if (g.part.has_local(root)) {
    auto const k = g.part.to_local(root);
    is_reached.set(k);
    active.push(k);
  }

  auto const on_fw_message = [&](auto u) {
    auto const l = g.part.to_local(u);
    if (is_undecided.get(l) && !is_reached.get(l)) {
      is_reached.set(l);
      active.push(l);
    }
  };
  auto const on_extern_fw_message = [&](edge_t e) {
    on_fw_message(e.u);
    cache.insert_or_assign(e.v, 1);
  };

  do {
    while (!active.empty()) {
      g.each_uv(active.pop_back(), [&](auto u, auto v) {
        if (g.part.has_local(v)) on_fw_message(v);
        else {
          cache.insert_or_assign(v, 1);
          fw_front.push(g.part, edge_t{ v, u });
        }
      });
    }
  } while (fw_front.comm(g.part, on_extern_fw_message));

  // backward search

  if (g.part.has_local(root)) {
    auto const k = g.part.to_local(root);
    is_reached.unset(k);
    DEBUG_ASSERT(is_undecided.get(k), "root must be undecided by definition");
    is_undecided.unset(k);
    on_decision(k, root);
    active.push(k);
  }

  auto const on_bw_message = [&](auto v) {
    auto const l = g.part.to_local(v);
    if (is_reached.get(l)) {
      is_reached.unset(l);
      DEBUG_ASSERT(is_undecided.get(l), "l should be undecided as is_reached protects it");
      is_undecided.unset(l);
      on_decision(l, root);
      active.push(l);
    }
  };
  auto const on_extern_bw_message = [&](edge_t e) {
    on_bw_message(e.u);
    cache.insert_or_assign(e.v, 0);
  };

  do {
    while (!active.empty()) {
      g.each_bw_uv(active.pop_back(), [&](auto u, auto v) {
        if (g.part.has_local(v)) on_bw_message(v);
        else {
          if (cache.get_default(v, 0)) {
            cache.assign(v, 0);
            bw_front.push(g.part, edge_t{ v, u });
          }
        }
      });
    }
  } while (bw_front.comm(g.part, on_extern_bw_message));
}

}
