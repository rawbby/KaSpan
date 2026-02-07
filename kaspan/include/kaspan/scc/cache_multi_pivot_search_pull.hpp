#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/bits_accessor.hpp>
#include <kaspan/memory/hash_map.hpp>
#include <kaspan/memory/stack_accessor.hpp>
#include <kaspan/scc/cache_multi_pivot_search.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/math.hpp>

namespace kaspan {

namespace internal_pull {

inline void
update_min_cache(
  hash_map<vertex_t>& cache,
  vertex_t            key,
  vertex_t            val,
  auto&&              on_update)
{
  auto const on_null = [&](auto& key_slot, auto& val_slot) {
    key_slot = key;
    val_slot = val;
    on_update();
  };
  auto const on_key = [&](auto& val_slot) {
    if (val <= val_slot) {
      val_slot = val;
      on_update();
    }
  };
  cache.find_key_or_null(key, on_key, on_null);
}

} // namespace internal_pull

template<part_view_c Part = single_part_view>
void
cache_label_search_pull(
  bidi_graph_part_view<Part> g,
  frontier&                  front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       has_changed_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision,
  auto&&                     map,
  auto&&                     unmap,
  hash_map<vertex_t>&        cache)
{
  auto* label        = label_storage;
  auto  active       = view_stack<vertex_t>(active_storage, g.part.local_n());
  auto  in_active    = view_bits(in_active_storage, g.part.local_n());
  auto  has_changed  = view_bits(has_changed_storage, g.part.local_n());
  auto  is_undecided = view_bits(is_undecided_storage, g.part.local_n());
  auto  fw_front     = front.view<labeled_edge_t>();
  auto  bw_front     = front.view<edge_t>();

  auto const bit_storage_count = ceildiv<64>(g.part.local_n());

  // forward search

  active.clear();
  in_active.clear(g.part.local_n());
  has_changed.clear(g.part.local_n());

  for (vertex_t k = 0; k < g.part.local_n(); ++k) {
    if (is_undecided.get(k)) {
      label[k] = map(g.part.to_global(k));
      in_active.set(k);
      active.push(k);
      has_changed.set(k);
    } else {
      label[k] = scc_id_undecided;
    }
  }

  auto const on_extern_fw_message = [&](labeled_edge_t e) {
    DEBUG_ASSERT(!g.part.has_local(e.l));
    internal_pull::update_min_cache(cache, e.l, e.v, [&] {
      auto const l = g.part.to_local(e.u);
      DEBUG_ASSERT(g.part.has_local(e.u));
      if (is_undecided.get(l) && !in_active.get(l)) {
        in_active.set(l);
        active.push(l);
      }
    });
  };

  do {
    while (!active.empty()) {
      auto const k = active.pop_back();
      if (!is_undecided.get(k)) {
        in_active.unset(k);
        continue;
      }

      vertex_t min_label = label[k];
      g.each_bw_v(k, [&](auto v) {
        if (g.part.has_local(v)) {
          auto const l = g.part.to_local(v);
          if (is_undecided.get(l)) {
            if (label[l] < min_label) {
              min_label = label[l];
            }
          }
        } else {
          cache.find_key_or_null(
            v,
            [&](auto val) {
              if (val < min_label) {
                min_label = val;
              }
            },
            [](auto&, auto&) {});
        }
      });

      if (min_label < label[k]) {
        label[k] = min_label;
        has_changed.set(k);
        g.each_v(k, [&](auto v) {
          if (g.part.has_local(v)) {
            auto const l = g.part.to_local(v);
            if (is_undecided.get(l) && !in_active.get(l)) {
              in_active.set(l);
              active.push(l);
            }
          }
        });
      }
      in_active.unset(k);
    }

    has_changed.each(g.part.local_n(), [&](auto&& k) {
      auto const label_k = label[k];
      g.each_v(k, [&](auto v) {
        if (label_k <= map(v) && !g.part.has_local(v)) {
          internal_pull::update_min_cache(cache, v, label_k, [&] { fw_front.push(g.part, labeled_edge_t{ v, label_k, g.part.to_global(k) }); });
        }
      });
    });
    has_changed.clear(g.part.local_n());

  } while (fw_front.comm(g.part, on_extern_fw_message));

  // backward search

  active.clear();
  in_active.clear(g.part.local_n());
  has_changed.clear(g.part.local_n());

  for (vertex_t k = 0; k < g.part.local_n(); ++k) {
    if (is_undecided.get(k) && label[k] == map(g.part.to_global(k))) {
      is_undecided.unset(k);
      on_decision(k, unmap(label[k]));
      has_changed.set(k);
    }
  }

  has_changed.each(g.part.local_n(), [&](auto k) {
    g.each_bw_v(k, [&](auto v) {
      if (g.part.has_local(v)) {
        auto const l = g.part.to_local(v);
        if (is_undecided.get(l) && !in_active.get(l)) {
          in_active.set(l);
          active.push(l);
        }
      }
    });
  });

  auto const on_bw_message = [&](edge_t e) {
    auto const l = g.part.to_local(e.u);
    if (is_undecided.get(l) && label[l] == e.v) {
      is_undecided.unset(l);
      on_decision(l, unmap(e.v));
      has_changed.set(l);
      g.each_bw_v(l, [&](auto v) {
        if (g.part.has_local(v)) {
          auto const neighbor_idx = g.part.to_local(v);
          if (is_undecided.get(neighbor_idx) && !in_active.get(neighbor_idx)) {
            in_active.set(neighbor_idx);
            active.push(neighbor_idx);
          }
        }
      });
    }
  };

  do {
    while (!active.empty()) {
      auto const k = active.pop_back();
      if (!is_undecided.get(k)) {
        in_active.unset(k);
        continue;
      }

      bool       reached = false;
      auto const label_k = label[k];
      g.each_v(k, [&](auto v) {
        if (g.part.has_local(v)) {
          auto const l = g.part.to_local(v);
          if (!is_undecided.get(l) && label[l] == label_k) {
            reached = true;
          }
        }
      });

      if (reached) {
        is_undecided.unset(k);
        on_decision(k, unmap(label_k));
        has_changed.set(k);
        g.each_bw_v(k, [&](auto v) {
          if (g.part.has_local(v)) {
            auto const l = g.part.to_local(v);
            if (is_undecided.get(l) && !in_active.get(l)) {
              in_active.set(l);
              active.push(l);
            }
          }
        });
      }
      in_active.unset(k);
    }

    has_changed.each(g.part.local_n(), [&](auto k) {
      auto const label_k = label[k];
      g.each_bw_v(k, [&](auto v) {
        if (label_k < map(v) && !g.part.has_local(v)) {
          if (label_k == cache.get_default(v, map(v))) {
            bw_front.push(g.part, edge_t{ v, label_k });
          }
        }
      });
    });
    has_changed.clear(g.part.local_n());

  } while (bw_front.comm(g.part, on_bw_message));
}

template<part_view_c Part = single_part_view>
void
cache_rot_label_search_pull(
  bidi_graph_part_view<Part> g,
  frontier&                  front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       has_changed_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision,
  hash_map<vertex_t>&        cache)
{
  static int rotation = 0;
  ++rotation;

  auto const map   = [](auto l) { return std::rotr(l, rotation); };
  auto const unmap = [](auto l) { return std::rotl(l, rotation); };

  cache_label_search_pull(g, front, label_storage, active_storage, in_active_storage, has_changed_storage, is_undecided_storage, on_decision, map, unmap, cache);
}

template<part_view_c Part = single_part_view>
void
cache_label_search_pull(
  bidi_graph_part_view<Part> g,
  frontier&                  front,
  vertex_t*                  label_storage,
  vertex_t*                  active_storage,
  u64*                       in_active_storage,
  u64*                       has_changed_storage,
  u64*                       is_undecided_storage,
  auto&&                     on_decision,
  hash_map<vertex_t>&        cache)
{
  auto const id = [](auto l) { return l; };
  cache_label_search_pull(g, front, label_storage, active_storage, in_active_storage, has_changed_storage, is_undecided_storage, on_decision, id, id, cache);
}

} // namespace kaspan
