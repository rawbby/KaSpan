#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <memory/accessor/stack_accessor.hpp>
#include <scc/base.hpp>
#include <scc/edge_frontier.hpp>
#include <scc/graph.hpp>
#include <scc/part.hpp>

#include <functional>

template<WorldPartConcept Part>
void
color_scc_init_label(Part const& part, vertex_t* colors)
{
  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) { colors[k] = part.to_global(k); }
}

template<WorldPartConcept Part>
auto
color_scc_step(Part const&     part,
               index_t const*  fw_head,
               vertex_t const* fw_csr,
               index_t const*  bw_head,
               vertex_t const* bw_csr,
               vertex_t*       scc_id,
               const vertex_t*       colors,
               vertex_t*       active_array,
               BitsAccessor    active,
               BitsAccessor    changed,
               edge_frontier&  frontier,
               vertex_t        decided_count = 0) -> vertex_t
{
  auto const local_n = part.local_n();

#if KASPAN_DEBUG
  // Validate decided_count is consistent with scc_id
  vertex_t actual_decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] != scc_id_undecided) { ++actual_decided_count; }
  }
  DEBUG_ASSERT_EQ(actual_decided_count, decided_count);
#endif

  auto active_stack = StackAccessor<vertex_t>{ active_array };

  // Phase 1: Forward Color Propagation (inspired by HPCGraph scc_color)
  // This partitions the graph into components that are supersets of SCCs.
  // Each vertex v gets the minimum ID of a vertex u that can reach it (u -> v).
  {
    active.fill_cmp(local_n, scc_id, scc_id_undecided);
    std::memcpy(changed.data(), active.data(), (local_n + 7) >> 3);
    active.for_each(local_n, [&](auto&& k) { active_stack.push(k); });

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
          if (label_k < v and not part.has_local(v)) { frontier.push(part.world_rank_of(v), { v, label_k }); }
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) { break; }

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
    active.fill_cmp(local_n, scc_id, scc_id_undecided);
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
          if (not part.has_local(v)) { frontier.push(part.world_rank_of(v), { v, pivot }); }
        }
      });
      changed.clear(local_n);

      if (not frontier.comm(part)) { break; }

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
