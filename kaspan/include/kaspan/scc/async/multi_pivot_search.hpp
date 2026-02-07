#pragma once

#include <kaspan/memory/bits_accessor.hpp>
#include <kaspan/memory/stack_accessor.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/util/math.hpp>

#include <cstring>

namespace kaspan::async {

template<part_view_c Part,
         typename brief_queue_t>
void
label_search(
  bidi_graph_part_view<Part> g,
  brief_queue_t&             front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision,
  auto&&                     map,
  auto&&                     unmap)
{
  auto* label        = label_storage;
  auto  active       = view_stack<vertex_t>(active_storage, g.part.local_n());
  auto  in_active    = view_bits(in_active_storage, g.part.local_n());
  auto  is_undecided = view_bits(is_undecided_storage, g.part.local_n());

  auto const bit_storage_count = ceildiv<64>(g.part.local_n());

  // forward search

  std::memcpy(in_active_storage, is_undecided_storage, bit_storage_count * sizeof(u64));
  in_active.each(g.part.local_n(), [&](auto k) {
    label[k] = map(g.part.to_global(k));
    active.push(k);
  });

  auto const on_fw_message = [&](edge_t e) {
    auto const [v, label_ku] = e;
    auto const l             = g.part.to_local(v);
    if (is_undecided.get(l) && label_ku < label[l]) {
      label[l] = label_ku;
      if (!in_active.get(l)) {
        in_active.set(l);
        active.push(l);
      }
    }
  };
  auto const on_fw_messages = [&](auto env) {
    for (auto e : env.message)
      on_fw_message(e);
  };

  front.reactivate();
  do {
    while (!active.empty()) {
      auto const k       = active.pop_back();
      auto const label_k = label[k];
      in_active.unset(k);

      g.each_v(k, [&](auto v) {
        if (label_k < map(v)) {
          if (g.part.has_local(v)) on_fw_message(edge_t{ v, label_k });
          else front.post_message_blocking(edge_t{ v, label_k }, g.part.world_rank_of(v), on_fw_messages);
        }
      });

      front.poll_throttled(on_fw_messages);
    }
  } while (!front.terminate(on_fw_messages));

  // backward search

  in_active.set_each(g.part.local_n(), [&](auto k) {
    if (is_undecided.get(k) && label[k] == map(g.part.to_global(k))) {
      is_undecided.unset(k);
      on_decision(k, unmap(label[k]));
      active.push(k);
      return true;
    }
    return false;
  });

  auto const on_bw_message = [&](edge_t e) {
    auto const [v, label_ku] = e;
    auto const l             = g.part.to_local(v);

    if (is_undecided.get(l) && label[l] == label_ku) {
      is_undecided.unset(l);
      on_decision(l, unmap(label_ku));
      if (!in_active.get(l)) {
        in_active.set(l);
        active.push(l);
      }
    }
  };
  auto const on_bw_messages = [&](auto env) {
    for (auto e : env.message)
      on_bw_message(e);
  };

  mpi_basic::barrier();

  front.reactivate();
  do {
    while (!active.empty()) {
      auto const k       = active.pop_back();
      auto const label_k = label[k];

      g.each_bw_v(k, [&](auto v) {
        if (label_k < map(v)) {
          if (g.part.has_local(v)) on_bw_message(edge_t{ v, label_k });
          else front.post_message_blocking(edge_t{ v, label_k }, g.part.world_rank_of(v), on_bw_messages);
        }
      });

      in_active.unset(k);
      front.poll_throttled(on_bw_messages);
    }
  } while (!front.terminate(on_bw_messages));
}

template<part_view_c Part,
         typename brief_queue_t>
void
rot_label_search(
  bidi_graph_part_view<Part> g,
  brief_queue_t&             front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision)
{
  static int rotation = 0;
  ++rotation;

  auto const map   = [](auto l) { return std::rotr(l, rotation); };
  auto const unmap = [](auto l) { return std::rotl(l, rotation); };

  label_search(g, front, label_storage, active_storage, in_active_storage, is_undecided_storage, on_decision, map, unmap);
}

template<part_view_c Part,
         typename brief_queue_t>
void
label_search(
  bidi_graph_part_view<Part> g,
  brief_queue_t&             front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision)
{
  auto const id = [](auto l) { return l; };
  label_search(g, front, label_storage, active_storage, in_active_storage, is_undecided_storage, on_decision, id, id);
}

} // namespace kaspan::async
