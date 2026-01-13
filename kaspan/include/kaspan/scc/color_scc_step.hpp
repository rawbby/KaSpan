#pragma once

#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/edge_frontier.hpp>
#include <kaspan/scc/graph.hpp>
#include <kaspan/scc/part.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

namespace kaspan {

template<world_part_concept part_t>
void
color_scc_init_label(part_t const& part, vertex_t* colors)
{
  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) {
    colors[k] = part.to_global(k);
  }
}

template<world_part_concept part_t>
auto
color_scc_step_multi(part_t const&   part,
                     index_t const*  fw_head,
                     vertex_t const* fw_csr,
                     index_t const*  bw_head,
                     vertex_t const* bw_csr,
                     vertex_t*       scc_id,
                     vertex_t*       colors,
                     vertex_t*       active_array,
                     u64*            active_storage,
                     u64*            changed_storage,
                     edge_frontier&  frontier,
                     vertex_t        local_pivot) -> vertex_t
{
  auto const local_n      = part.local_n();
  auto       active       = view_bits(active_storage, local_n);
  auto       changed      = view_bits(changed_storage, local_n);
  auto       active_stack = view_stack<vertex_t>(active_array, local_n);

  auto const count_degree = [=](vertex_t k, index_t const* head, vertex_t const* csr) {
    index_t degree = 0;
    for (auto u : csr_range(head, csr, k)) {
      degree += integral_cast<index_t>(not part.has_local(u) || scc_id[part.to_local(u)] == scc_id_undecided);
    }
    return degree;
  };

  // Phase 1: Forward Color Propagation (inspired by HPCGraph scc_color)
  // This partitions the graph into components that are supersets of SCCs.
  // Each vertex v gets the minimum ID of a vertex u that can reach it (u -> v).
  {
    if (local_pivot != local_n and scc_id[local_pivot] == scc_id_undecided) {
      colors[local_pivot] = part.to_global(local_pivot);
      active.set(local_pivot);
      changed.set(local_pivot);
      active_stack.push(local_pivot);
    }

    while (true) {
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();

        auto const label = colors[k];
        for (auto v : csr_range(fw_head, fw_csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and label < colors[l]) {
              colors[l] = label;
              changed.set(l);
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }
        active.unset(k);
      }

      changed.for_each(local_n, [&](auto&& k) {
        auto const label_k = colors[k];
        for (auto v : csr_range(fw_head, fw_csr, k)) {
          // if (label_k < v and not part.has_local(v)) {
          frontier.push(part.world_rank_of(v), { v, label_k });
          //}
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, label] = frontier.next();
        auto const k          = part.to_local(u);
        if (scc_id[k] == scc_id_undecided and label < colors[k]) {
          colors[k] = label;
          changed.set(k);
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
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
    DEBUG_ASSERT(active_stack.empty());

    if (local_pivot != local_n and scc_id[local_pivot] == scc_id_undecided and colors[local_pivot] == part.to_global(local_pivot)) {
      active.set(local_pivot);
      changed.set(local_pivot);
      active_stack.push(local_pivot);

      scc_id[local_pivot] = colors[local_pivot];
      ++local_decided_count;
    }

    while (true) {
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();

        auto const pivot = colors[k];
        for (auto v : csr_range(bw_head, bw_csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and colors[l] == pivot) {
              scc_id[l] = pivot;
              ++local_decided_count;
              changed.set(l);
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }

        active.unset(k);
      }

      changed.for_each(local_n, [&](auto&& k) {
        auto const pivot = colors[k];
        for (auto v : csr_range(bw_head, bw_csr, k)) {
          if (not part.has_local(v)) {
            frontier.push(part.world_rank_of(v), { v, pivot });
          }
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, pivot] = frontier.next();
        auto const k          = part.to_local(u);
        if (scc_id[k] == scc_id_undecided and colors[k] == pivot) {
          scc_id[k] = pivot;
          ++local_decided_count;
          changed.set(k);
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
          }
        }
      }
    }
  }

  return local_decided_count;
}

template<world_part_concept part_t>
auto
color_scc_step_multi(part_t const&   part,
                     index_t const*  fw_head,
                     vertex_t const* fw_csr,
                     index_t const*  bw_head,
                     vertex_t const* bw_csr,
                     vertex_t*       scc_id,
                     vertex_t*       colors,
                     vertex_t*       active_array,
                     u64*            active_storage,
                     u64*            changed_storage,
                     edge_frontier&  frontier) -> vertex_t
{
  auto const local_n = part.local_n();
  auto       active  = view_bits(active_storage, local_n);
  auto       changed = view_bits(changed_storage, local_n);

  auto const count_degree = [=](vertex_t k, index_t const* head, vertex_t const* csr) {
    index_t degree = 0;
    for (auto u : csr_range(head, csr, k)) {
      degree += integral_cast<index_t>(not part.has_local(u) || scc_id[part.to_local(u)] == scc_id_undecided);
    }
    return degree;
  };

  vertex_t local_decided_count = 0;

  index_t  max_degree_product = 0;
  vertex_t max_degree_vertex  = local_n;

  for (vertex_t k = 0; k < local_n; ++k) {
    colors[k] = part.n;

    if (scc_id[k] == scc_id_undecided) {
      auto const out_degree = count_degree(k, fw_head, fw_csr);

      if (out_degree == 0) {
        scc_id[k] = part.to_global(k);
        ++local_decided_count;
        continue;
      }

      auto const in_degree = count_degree(k, bw_head, bw_csr);
      if (in_degree == 0) {
        scc_id[k] = part.to_global(k);
        ++local_decided_count;
        continue;
      }

      auto const degree_product = in_degree * out_degree;
      if (degree_product > max_degree_product) {
        max_degree_product = degree_product;
        max_degree_vertex  = k;
      }

      // if (in_degree * out_degree >= threshold) {
      //   scc_id[k] = part.to_global(k);
      //   ++local_decided_count;
      // }
    }
  }

  return local_decided_count + color_scc_step_multi(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, colors, active_array, active, changed, frontier, max_degree_vertex);
}

template<world_part_concept part_t>
auto
color_scc_step(part_t const&   part,
               index_t const*  fw_head,
               vertex_t const* fw_csr,
               index_t const*  bw_head,
               vertex_t const* bw_csr,
               vertex_t*       scc_id,
               vertex_t*       colors,
               vertex_t*       active_array,
               u64*            active_storage,
               u64*            changed_storage,
               edge_frontier&  frontier,
               vertex_t        decided_count = 0) -> vertex_t
{
  auto const local_n      = part.local_n();
  auto       active       = view_bits(active_storage, local_n);
  auto       changed      = view_bits(changed_storage, local_n);
  auto       active_stack = view_stack<vertex_t>(active_array, local_n - decided_count);

#if KASPAN_DEBUG
  // Validate decided_count is consistent with scc_id
  vertex_t actual_decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] != scc_id_undecided) {
      ++actual_decided_count;
    }
  }
  DEBUG_ASSERT_EQ(actual_decided_count, decided_count);
#endif

  for (vertex_t k = 0; k < local_n; ++k) {
    colors[k] = part.to_global(k);
  }

  // Phase 1: Forward Color Propagation (inspired by HPCGraph scc_color)
  // This partitions the graph into components that are supersets of SCCs.
  // Each vertex v gets the minimum ID of a vertex u that can reach it (u -> v).
  {
    active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    std::memcpy(changed.data(), active.data(), (local_n + 7) >> 3);
    active.for_each(local_n, [&](auto&& k) {
      active_stack.push(k);
    });

    while (true) {
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();

        auto const label = colors[k];
        for (auto v : csr_range(fw_head, fw_csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and label < colors[l]) {
              colors[l] = label;
              changed.set(l);
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }
        active.unset(k);
      }

      changed.for_each(local_n, [&](auto&& k) {
        auto const label_k = colors[k];
        for (auto v : csr_range(fw_head, fw_csr, k)) {
          if (label_k < v and not part.has_local(v)) {
            frontier.push(part.world_rank_of(v), { v, label_k });
          }
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, label] = frontier.next();
        auto const k          = part.to_local(u);
        if (scc_id[k] == scc_id_undecided and label < colors[k]) {
          colors[k] = label;
          changed.set(k);
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
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
    DEBUG_ASSERT(active_stack.empty());
    active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    changed.clear(local_n);
    active.for_each(local_n, [&](auto k) {
      if (colors[k] == part.to_global(k)) {
        scc_id[k] = colors[k];
        ++local_decided_count;
        changed.set(k);
        active_stack.push(k);
      } else {
        active.unset(k);
      }
    });

    while (true) {
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();
        active.unset(k);

        auto const pivot = colors[k];
        for (auto v : csr_range(bw_head, bw_csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and colors[l] == pivot) {
              scc_id[l] = pivot;
              ++local_decided_count;
              changed.set(l);
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }
      }

      changed.for_each(local_n, [&](auto&& k) {
        auto const pivot = colors[k];
        for (auto v : csr_range(bw_head, bw_csr, k)) {
          if (not part.has_local(v)) {
            frontier.push(part.world_rank_of(v), { v, pivot });
          }
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, pivot] = frontier.next();
        auto const k          = part.to_local(u);
        if (scc_id[k] == scc_id_undecided and colors[k] == pivot) {
          scc_id[k] = pivot;
          ++local_decided_count;
          changed.set(k);
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
          }
        }
      }
    }
  }

  return local_decided_count;
}

} // namespace kaspan
