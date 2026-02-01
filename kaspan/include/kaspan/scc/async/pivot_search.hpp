#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>

namespace kaspan::async {

template<part_view_concept Part,
         typename brief_queue_t>
void
forward_backward_search(
  bidi_graph_part_view<Part> g,
  brief_queue_t&             front,
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

  auto const on_fw_message = [&](vertex_t v) {
    auto const l = g.part.to_local(v);
    if (!is_reached.get(l) && is_undecided.get(l)) {
      is_reached.set(l);
      active.push(l);
    }
  };
  auto on_fw_messages = [&](auto env) {
    for (auto v : env.message)
      on_fw_message(v);
  };

  if (g.part.has_local(root)) {
    auto const k = g.part.to_local(root);
    is_reached.set(k);
    active.push(k);
  }

  front.reactivate();
  do {
    while (!active.empty()) {
      g.each_v(active.pop_back(), [&](auto v) {
        if (g.part.has_local(v)) on_fw_message(v);
        else front.post_message_blocking(v, g.part.world_rank_of(v), on_fw_messages);
      });
      front.poll(on_fw_messages);
    }
  } while (!front.terminate(on_fw_messages));

  // backward search

  auto const on_bw_message = [&](vertex_t v) {
    auto const l = g.part.to_local(v);
    if (is_reached.get(l)) {
      is_reached.unset(l);
      DEBUG_ASSERT(is_undecided.get(l), "l should be undecided as is_reached protects it");
      is_undecided.unset(l);
      on_decision(l, root);
      active.push(l);
    }
  };
  auto on_bw_messages = [&](auto env) {
    for (auto v : env.message)
      on_bw_message(v);
  };

  if (g.part.has_local(root)) {
    auto const k = g.part.to_local(root);
    is_reached.unset(k);
    DEBUG_ASSERT(is_undecided.get(k), "root must be undecided by definition");
    is_undecided.unset(k);
    on_decision(k, root);
    active.push(k);
  }

  mpi_basic::barrier();

  front.reactivate();
  do {
    while (!active.empty()) {
      auto const k = active.pop_back();
      DEBUG_ASSERT_NOT(is_undecided.get(k));
      g.each_bw_v(k, [&](auto v) {
        if (g.part.has_local(v)) on_bw_message(v);
        else front.post_message_blocking(v, g.part.world_rank_of(v), on_bw_messages);
      });
      front.poll(on_bw_messages);
    }
  } while (!front.terminate(on_bw_messages));
}

} // namespace kaspan::async
