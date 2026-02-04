#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/hash_index_map.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/math.hpp>

namespace kaspan {

template<part_view_concept Part = single_part_view>
void
cache_label_search(
  bidi_graph_part_view<Part>    g,
  frontier&                     front,
  vertex_t*                     label_storage,
  vertex_t*                     active_storage,
  u64*                          in_active_storage,
  u64*                          has_changed_storage,
  u64*                          is_undecided_storage,
  auto&&                        on_decision,
  auto&&                        map,
  auto&&                        unmap,
  hash_index_map_view<vertex_t> cache_index,
  vertex_t*                     label_cache)
{
  auto* label        = label_storage;
  auto  active       = view_stack<vertex_t>(active_storage, g.part.local_n());
  auto  in_active    = view_bits(in_active_storage, g.part.local_n());
  auto  has_changed  = view_bits(has_changed_storage, g.part.local_n());
  auto  is_undecided = view_bits(is_undecided_storage, g.part.local_n());
  auto  fw_front     = front.view<labeled_edge_t>();
  auto  bw_front     = front.view<edge_t>();

  auto const bit_storage_count = ceildiv<64>(g.part.local_n());

  auto const init_cache = [&](auto v) {
    if (!g.part.has_local(v)) label_cache[cache_index.get(v)] = map(v);
  };
  is_undecided.each(g.part.local_n(), [&](auto k) { g.each_v(k, init_cache); });
  is_undecided.each(g.part.local_n(), [&](auto k) { g.each_bw_v(k, init_cache); });

  // forward search

  std::memcpy(has_changed_storage, is_undecided_storage, bit_storage_count * sizeof(u64));
  std::memcpy(in_active_storage, is_undecided_storage, bit_storage_count * sizeof(u64));
  in_active.each(g.part.local_n(), [&](auto k) {
    label[k] = map(g.part.to_global(k));
    active.push(k);
  });

  auto const on_fw_message = [&](labeled_edge_t e) {
    auto const l = g.part.to_local(e.u);
    if (is_undecided.get(l)) {
      if (e.v < label[l]) {
        label[l] = e.v;
        if (!in_active.get(l)) {
          in_active.set(l);
          active.push(l);
          has_changed.set(l);
        }
      }
    }
  };
  auto const on_extern_fw_message = [&](labeled_edge_t e) {
    DEBUG_ASSERT(!g.part.has_local(e.l));
    on_fw_message(e);
    auto& cache_label = label_cache[cache_index.get(e.l)];
    if (e.v < cache_label) {
      cache_label = e.v;
    }
  };

  do {
    while (!active.empty()) {
      auto const k       = active.pop_back();
      auto const label_k = label[k];
      g.each_v(k, [&](auto v) {
        if (label_k < map(v) && g.part.has_local(v)) on_fw_message(labeled_edge_t{ v, label_k, g.part.to_global(k) });
      });
      in_active.unset(k);
    }

    has_changed.each(g.part.local_n(), [&](auto&& k) {
      auto const label_k = label[k];
      g.each_v(k, [&](auto v) {
        if (label_k <= map(v) && !g.part.has_local(v) && label_k <= label_cache[cache_index.get(v)]) {
          label_cache[cache_index.get(v)] = label_k;
          fw_front.push(g.part, labeled_edge_t{ v, label_k, g.part.to_global(k) });
        }
      });
    });
    memset(has_changed_storage, 0x00, bit_storage_count * sizeof(u64));

  } while (fw_front.comm(g.part, on_extern_fw_message));

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
    auto const l = g.part.to_local(e.u);
    if (is_undecided.get(l) && label[l] == e.v) {
      is_undecided.unset(l);
      on_decision(l, unmap(e.v));
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

    has_changed.each(g.part.local_n(), [&](auto k) {
      auto const label_k = label[k];
      g.each_bw_v(k, [&](auto v) {
        if (label_k < map(v) && !g.part.has_local(v) && label_k == label_cache[cache_index.get(v)]) {
          bw_front.push(g.part, edge_t{ v, label_k });
        }
      });
    });
    memset(has_changed_storage, 0x00, bit_storage_count * sizeof(u64));

  } while (bw_front.comm(g.part, on_bw_message));
}

template<part_view_concept Part = single_part_view>
void
cache_rot_label_search(
  bidi_graph_part_view<Part>    g,
  frontier& front,
  vertex_t*                     label_storage,
  vertex_t*                     active_storage,
  u64*                          in_active_storage,
  u64*                          has_changed_storage,
  u64*                          is_undecided_storage,
  auto&&                        on_decision,
  hash_index_map_view<vertex_t> cache_index,
  vertex_t*                     label_cache)
{
  static int rotation = 0;
  ++rotation;

  auto const map   = [](auto l) { return std::rotr(l, rotation); };
  auto const unmap = [](auto l) { return std::rotl(l, rotation); };

  cache_label_search(g, front, label_storage, active_storage, in_active_storage, has_changed_storage, is_undecided_storage, on_decision, map, unmap, cache_index, label_cache);
}

template<part_view_concept Part = single_part_view>
void
cache_label_search(
  bidi_graph_part_view<Part>    g,
  frontier& front,
  vertex_t*                     label_storage,
  vertex_t*                     active_storage,
  u64*                          in_active_storage,
  u64*                          has_changed_storage,
  u64*                          is_undecided_storage,
  auto&&                        on_decision,
  hash_index_map_view<vertex_t> cache_index,
  vertex_t*                     label_cache)
{
  auto const id = [](auto l) { return l; };
  cache_label_search(g, front, label_storage, active_storage, in_active_storage, has_changed_storage, is_undecided_storage, on_decision, id, id, cache_index, label_cache);
}

}
