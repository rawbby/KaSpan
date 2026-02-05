#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/math.hpp>

namespace kaspan {

template<part_view_c Part = single_part_view>
void
label_search(
  bidi_graph_part_view<Part> g,
  frontier_view<edge_t>      front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       has_changed_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision,
  auto&&                     map,
  auto&&                     unmap)
{
  auto* label        = label_storage;
  auto  active       = view_stack<vertex_t>(active_storage, g.part.local_n());
  auto  in_active    = view_bits(in_active_storage, g.part.local_n());
  auto  has_changed  = view_bits(has_changed_storage, g.part.local_n());
  auto  is_undecided = view_bits(is_undecided_storage, g.part.local_n());

  auto const bit_storage_count = ceildiv<64>(g.part.local_n());

  // forward search

  std::memcpy(has_changed_storage, is_undecided_storage, bit_storage_count * sizeof(u64));
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
        has_changed.set(l);
      }
    }
  };

  do {
    while (!active.empty()) {
      auto const k       = active.pop_back();
      auto const label_k = label[k];

      g.each_v(k, [&](auto v) {
        if (label_k < map(v) && g.part.has_local(v)) on_fw_message(edge_t{ v, label_k });
      });

      in_active.unset(k);
    }

    has_changed.each(g.part.local_n(), [&](auto&& k) {
      auto const label_k = label[k];
      g.each_v(k, [&](auto v) {
        if (label_k < map(v) && !g.part.has_local(v)) front.push(g.part, edge_t{ v, label_k });
      });
    });
    memset(has_changed_storage, 0x00, bit_storage_count * sizeof(u64));

  } while (front.comm(g.part, on_fw_message));

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

  std::memcpy(has_changed_storage, in_active_storage, bit_storage_count * sizeof(u64));

  auto const on_bw_message = [&](edge_t e) {
    auto const [v, label_ku] = e;
    auto const l             = g.part.to_local(v);

    if (is_undecided.get(l) && label[l] == label_ku) {
      is_undecided.unset(l);
      on_decision(l, unmap(label_ku));
      if (!in_active.get(l)) {
        in_active.set(l);
        active.push(l);
        has_changed.set(l);
      }
    }
  };

  do {
    while (!active.empty()) {
      auto const k       = active.pop_back();
      auto const label_k = label[k];

      g.each_bw_v(k, [&](auto v) {
        if (label_k < map(v) && g.part.has_local(v)) on_bw_message(edge_t{ v, label_k });
      });

      in_active.unset(k);
    }

    has_changed.each(g.part.local_n(), [&](auto&& k) {
      auto const label_k = label[k];
      g.each_bw_v(k, [&](auto v) {
        if (label_k < map(v) && !g.part.has_local(v)) front.push(g.part, edge_t{ v, label_k });
      });
    });
    memset(has_changed_storage, 0x00, bit_storage_count * sizeof(u64));

  } while (front.comm(g.part, on_bw_message));
}

template<part_view_c Part = single_part_view>
void
rot_label_search(
  bidi_graph_part_view<Part> g,
  frontier_view<edge_t>      front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       has_changed_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision)
{
  static int rotation = 0;
  ++rotation;

  auto const map   = [](auto l) { return std::rotr(l, rotation); };
  auto const unmap = [](auto l) { return std::rotl(l, rotation); };

  label_search(g, front, label_storage, active_storage, in_active_storage, has_changed_storage, is_undecided_storage, on_decision, map, unmap);
}

template<part_view_c Part = single_part_view>
void
label_search(
  bidi_graph_part_view<Part> g,
  frontier_view<edge_t>      front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       has_changed_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision)
{
  auto const id = [](auto l) { return l; };
  label_search(g, front, label_storage, active_storage, in_active_storage, has_changed_storage, is_undecided_storage, on_decision, id, id);
}

}
