#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

namespace kaspan {

// /// start with up to one pivot per rank
// template<u64               Threshold,
//          part_view_concept Part>
// auto
// color_scc_step_multi(
//   bidi_graph_part_view<Part> graph,
//   vertex_t*                  scc_id,
//   vertex_t*                  colors,
//   vertex_t*                  active_array,
//   u64*                       active_storage,
//   u64*                       changed_storage,
//   frontier_view<edge_t,
//                 Threshold>   frontier,
//   vertex_t                   local_pivot,
//   auto&&                     on_decision = [](vertex_t) {}) -> vertex_t
// {
//   auto       part         = graph.part;
//   auto const local_n      = part.local_n();
//   auto       active       = view_bits(active_storage, local_n);
//   auto       changed      = view_bits(changed_storage, local_n);
//   auto       active_stack = view_stack<vertex_t>(active_array, local_n);
//
//   // Phase 1: Forward Color Propagation (inspired by HPCGraph scc_color)
//   // This partitions the graph into components that are supersets of SCCs.
//   // Each vertex v gets the minimum ID of a vertex u that can reach it (u -> v).
//   {
//     if (local_pivot != local_n && scc_id[local_pivot] == scc_id_undecided) {
//       colors[local_pivot] = part.to_global(local_pivot);
//       active.set(local_pivot);
//       changed.set(local_pivot);
//       active_stack.push(local_pivot);
//     }
//
//     while (true) {
//       while (!active_stack.empty()) {
//         auto const k = active_stack.back();
//         active_stack.pop();
//
//         auto const label = colors[k];
//         for (auto v : graph.csr_range(k)) {
//           if (part.has_local(v)) {
//             auto const l = part.to_local(v);
//             if (scc_id[l] == scc_id_undecided && label < colors[l]) {
//               colors[l] = label;
//               changed.set(l);
//               if (!active.get(l)) {
//                 active.set(l);
//                 active_stack.push(l);
//               }
//             }
//           }
//         }
//         active.unset(k);
//       }
//
//       changed.for_each(local_n, [&](auto&& k) {
//         auto const label_k = colors[k];
//         for (auto v : graph.csr_range(k)) {
//           // if (label_k < v && ! part.has_local(v)) {
//           frontier.push(part, part.world_rank_of(v), { v, label_k });
//           //}
//         }
//       });
//       changed.clear(local_n);
//
//       if (!frontier.comm(part)) {
//         break;
//       }
//
//       while (frontier.has_next()) {
//         auto const [u, label] = frontier.next();
//         auto const k          = part.to_local(u);
//         if (scc_id[k] == scc_id_undecided && label < colors[k]) {
//           colors[k] = label;
//           changed.set(k);
//           if (!active.get(k)) {
//             active.set(k);
//             active_stack.push(k);
//           }
//         }
//       }
//     }
//   }
//
//   // Phase 2: Multi-pivot Backward Search (inspired by HPCGraph scc_find_sccs)
//   // For each color/component, the vertex u where colors[u] == u is the pivot.
//   // We perform a backward search from these pivots, restricted to the same color.
//   // Intersection of forward reaching (Phase 1) and backward reaching (Phase 2) is the SCC.
//   vertex_t local_decided_count = 0;
//   {
//     DEBUG_ASSERT(active_stack.empty());
//
//     if (local_pivot != local_n && scc_id[local_pivot] == scc_id_undecided && colors[local_pivot] == part.to_global(local_pivot)) {
//       active.set(local_pivot);
//       changed.set(local_pivot);
//       active_stack.push(local_pivot);
//
//       scc_id[local_pivot] = colors[local_pivot];
//       on_decision(local_pivot);
//       ++local_decided_count;
//     }
//
//     while (true) {
//       while (!active_stack.empty()) {
//         auto const k = active_stack.back();
//         active_stack.pop();
//
//         auto const pivot = colors[k];
//         for (auto v : graph.bw_csr_range(k)) {
//           if (part.has_local(v)) {
//             auto const l = part.to_local(v);
//             if (scc_id[l] == scc_id_undecided && colors[l] == pivot) {
//               scc_id[l] = pivot;
//               on_decision(l);
//               ++local_decided_count;
//               changed.set(l);
//               if (!active.get(l)) {
//                 active.set(l);
//                 active_stack.push(l);
//               }
//             }
//           }
//         }
//
//         active.unset(k);
//       }
//
//       changed.for_each(local_n, [&](auto&& k) {
//         auto const pivot = colors[k];
//         for (auto v : graph.bw_csr_range(k)) {
//           if (!part.has_local(v)) {
//             frontier.push(part, part.world_rank_of(v), { v, pivot });
//           }
//         }
//       });
//       changed.clear(local_n);
//
//       if (!frontier.comm(part)) {
//         break;
//       }
//
//       while (frontier.has_next()) {
//         auto const [u, pivot] = frontier.next();
//         auto const k          = part.to_local(u);
//         if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
//           scc_id[k] = pivot;
//           on_decision(k);
//           ++local_decided_count;
//           changed.set(k);
//           if (!active.get(k)) {
//             active.set(k);
//             active_stack.push(k);
//           }
//         }
//       }
//     }
//   }
//
//   return local_decided_count;
// }

// template<u64               Threshold,
//          part_view_concept Part>
// auto
// color_scc_step_multi(
//   bidi_graph_part_view<Part> graph,
//   vertex_t*                  scc_id,
//   vertex_t*                  colors,
//   vertex_t*                  active_array,
//   u64*                       active_storage,
//   u64*                       changed_storage,
//   frontier_view<edge_t,
//                 Threshold>   frontier,
//   auto&&                     on_decision = [](vertex_t) {}) -> vertex_t
// {
//   auto       part    = graph.part;
//   auto const local_n = part.local_n();
//   auto       active  = view_bits(active_storage, local_n);
//   auto       changed = view_bits(changed_storage, local_n);
//
//   auto const count_degree = [=, &part](vertex_t k, graph_view const g) {
//     vertex_t degree = 0;
//     for (auto u : g.csr_range(k)) {
//       degree += integral_cast<vertex_t>(!part.has_local(u) || scc_id[part.to_local(u)] == scc_id_undecided);
//     }
//     return degree;
//   };
//
//   vertex_t local_decided_count = 0;
//
//   index_t  max_degree_product = 0;
//   vertex_t max_degree_vertex  = local_n;
//
//   for (vertex_t k = 0; k < local_n; ++k) {
//     colors[k] = part.n();
//
//     if (scc_id[k] == scc_id_undecided) {
//       auto const out_degree = count_degree(k, graph.fw_view());
//
//       if (out_degree == 0) {
//         scc_id[k] = part.to_global(k);
//         on_decision(k);
//         ++local_decided_count;
//         continue;
//       }
//
//       auto const in_degree = count_degree(k, graph.bw_view());
//       if (in_degree == 0) {
//         scc_id[k] = part.to_global(k);
//         on_decision(k);
//         ++local_decided_count;
//         continue;
//       }
//
//       auto const degree_product = integral_cast<index_t>(in_degree) * out_degree;
//       if (degree_product > max_degree_product) {
//         max_degree_product = degree_product;
//         max_degree_vertex  = k;
//       }
//
//       // if (in_degree * out_degree >= threshold) {
//       //   scc_id[k] = part.to_global(k);
//       //   ++local_decided_count;
//       // }
//     }
//   }
//
//   return local_decided_count + color_scc_step_multi(graph, scc_id, colors, active_array, active_storage, changed_storage, frontier, max_degree_vertex, on_decision);
// }

template<u64               Threshold,
         part_view_concept Part>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  frontier_view<edge_t,
                Threshold>   frontier,
  vector<vertex_t>&          active,
  u64*                       is_active_storage,
  u64*                       is_changed_storage,
  vertex_t*                  colors,
  vertex_t*                  scc_id,
  auto&&                     on_decision = [](vertex_t) {}) -> vertex_t
{
  auto       part    = graph.part;
  auto const local_n = part.local_n();

  auto is_active  = view_bits(is_active_storage, local_n);
  auto is_changed = view_bits(is_changed_storage, local_n);

  for (vertex_t k = 0; k < local_n; ++k) {
    colors[k] = part.to_global(k);
  }

  // Phase 1: Forward Color Propagation (inspired by HPCGraph scc_color)
  // This partitions the graph into components that are supersets of SCCs.
  // Each vertex v gets the minimum ID of a vertex u that can reach it (u -> v).
  {
    is_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    std::memcpy(is_changed.data(), is_active.data(), (local_n + 7) >> 3);
    is_active.for_each(local_n, [&](auto&& k) { active.push_back(k); });

    while (true) {
      while (!active.empty()) {
        auto const k = active.back();
        active.pop_back();

        auto const label = colors[k];
        for (auto v : graph.csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided && label < colors[l]) {
              colors[l] = label;
              is_changed.set(l);
              if (!is_active.get(l)) {
                is_active.set(l);
                active.push_back(l);
              }
            }
          }
        }
        is_active.unset(k);
      }

      is_changed.for_each(local_n, [&](auto&& k) {
        auto const label_k = colors[k];
        for (auto v : graph.csr_range(k)) {
          if (label_k < v && !part.has_local(v)) {
            frontier.push(part, part.world_rank_of(v), { v, label_k });
          }
        }
      });
      is_changed.clear(local_n);

      if (!frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, label] = frontier.next();
        auto const k          = part.to_local(u);
        if (scc_id[k] == scc_id_undecided && label < colors[k]) {
          colors[k] = label;
          is_changed.set(k);
          if (!is_active.get(k)) {
            is_active.set(k);
            active.push_back(k);
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
    DEBUG_ASSERT(active.empty());
    is_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    is_changed.clear(local_n);
    is_active.for_each(local_n, [&](auto k) {
      if (colors[k] == part.to_global(k)) {
        scc_id[k] = colors[k];
        on_decision(k);
        ++local_decided_count;
        is_changed.set(k);
        active.push_back(k);
      } else {
        is_active.unset(k);
      }
    });

    while (true) {
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
              on_decision(l);
              ++local_decided_count;
              is_changed.set(l);
              if (!is_active.get(l)) {
                is_active.set(l);
                active.push_back(l);
              }
            }
          }
        }
      }

      is_changed.for_each(local_n, [&](auto&& k) {
        auto const pivot = colors[k];
        for (auto v : graph.bw_csr_range(k)) {
          if (!part.has_local(v)) {
            frontier.push(part, part.world_rank_of(v), { v, pivot });
          }
        }
      });
      is_changed.clear(local_n);

      if (!frontier.comm(part)) {
        break;
      }

      while (frontier.has_next()) {
        auto const [u, pivot] = frontier.next();
        auto const k          = part.to_local(u);
        if (scc_id[k] == scc_id_undecided && colors[k] == pivot) {
          scc_id[k] = pivot;
          on_decision(k);
          ++local_decided_count;
          is_changed.set(k);
          if (!is_active.get(k)) {
            is_active.set(k);
            active.push_back(k);
          }
        }
      }
    }
  }

  return local_decided_count;
}

template<u64               Threshold,
         part_view_concept Part>
auto
color_scc_step(
  bidi_graph_part_view<Part> graph,
  frontier_view<edge_t,
                Threshold>   frontier,
  vector<vertex_t>&          active,
  u64*                       is_active_storage,
  u64*                       is_changed_storage,
  vertex_t*                  colors,
  vertex_t*                  scc_id) -> vertex_t
{
  return color_scc_step(graph, frontier, active, is_active_storage, is_changed_storage, colors, scc_id, [](auto /* v */) {});
}

} // namespace kaspan
