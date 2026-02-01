#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/math.hpp>

#include <cstring>

namespace kaspan {

template<part_view_concept Part = single_part_view>
void
forward_backward_search(
  bidi_graph_part_view<Part> g,
  frontier_view<vertex_t>    front,
  vertex_t*                  active_storage,
  u64*                       is_reached_storage,
  u64*                       is_undecided_storage,
  vertex_t                   root,
  auto&&                     on_decision)
{
  auto active       = view_stack<vertex_t>(active_storage, g.part.local_n());
  auto is_reached   = view_bits_clean(is_reached_storage, g.part.local_n());
  auto is_undecided = view_bits(is_undecided_storage, g.part.local_n());

  // forward search

  if (g.part.has_local(root)) {
    auto const k = g.part.to_local(root);
    is_reached.set(k);
    active.push(k);
  }

  auto const on_fw_message = [&](auto v) {
    auto const l = g.part.to_local(v);
    if (!is_reached.get(l) && is_undecided.get(l)) {
      is_reached.set(l);
      active.push(l);
    }
  };

  do {
    while (!active.empty()) {
      g.each_v(active.pop_back(), [&](auto v) {
        if (g.part.has_local(v)) on_fw_message(v);
        else front.push(g.part, v);
      });
    }
  } while (front.comm(g.part, on_fw_message));

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

  do {
    while (!active.empty()) {
      g.each_bw_v(active.pop_back(), [&](auto v) {
        if (g.part.has_local(v)) on_bw_message(v);
        else front.push(g.part, v);
      });
    }
  } while (front.comm(g.part, on_bw_message));
}

}
