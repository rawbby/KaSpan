#pragma once

#include <kaspan/debug/statistic.hpp>
#include <kaspan/util/scope_guard.hpp>

// template<world_part_concept part_t>
// inline auto
// wcc(part_t const& part,
//
//     index_t const*  fw_head,
//     vertex_t const*  fw_csr,
//     index_t const*  bw_head,
//     vertex_t const*  bw_csr,
//
//     vertex_t*  wcc_id,
//     void*  memory) -> u64
// {
//   KASPAN_STATISTIC_SCOPE("wcc");
//
//   auto const local_n  = part.local_n();
//   auto       wcc_work = borrow<vertex_t>(memory, local_n);
//
// #pragma omp parallel for schedule(static)
//   for (vertex_t u = 0; u < local_n; ++u)
//     wcc_work[u] = u;
//
//   while (true) {
//     bool changed = false;
//
// #pragma omp parallel for schedule(static) reduction(|| : changed)
//     for (vertex_t k = 0; k < local_n; ++k) {
//       auto best = wcc_work[k];
//
//       {
//         auto const beg = fw_head[k];
//         auto const end = fw_head[k + 1];
//         for (index_t it = beg; it < end; ++it)
//           if (auto const v = fw_csr[it]; part.has_local(v))
//             best = std::min(best, wcc_work[part.to_local(v)]);
//       }
//
//       {
//         auto const beg = bw_head[k];
//         auto const end = bw_head[k + 1];
//         for (index_t it = beg; it < end; ++it)
//           if (auto const v = bw_csr[it]; part.has_local(v))
//             best = std::min(best, wcc_work[part.to_local(v)]);
//       }
//
//       wcc_best[k] = best;
//       if (best != wcc_work[k])
//         changed = true;
//     }
//
//     if (not changed)
//       break;
//
//     std::swap(wcc_work, wcc_best);
//   }
// }

namespace kaspan {

inline auto
wcc(vertex_t n, index_t* fw_head, index_t* fw_csr, vertex_t* wcc_id) -> size_t
{
  KASPAN_STATISTIC_SCOPE("residual_wcc");

  // propagate min wcc_id along fw and bw edges until convergence
  while (true) {

    bool wcc_id_changed = false;

    for (vertex_t u = 0; u < n; ++u) {
      auto const beg = fw_head[u];
      auto const end = fw_head[u + 1];
      for (auto it = beg; it < end; ++it) {
        auto const v = fw_csr[it];

        if (wcc_id[v] < wcc_id[u]) {
          wcc_id[u]      = wcc_id[v];
          wcc_id_changed = true;
        } else if (wcc_id[v] > wcc_id[u]) {
          wcc_id[v]      = wcc_id[u];
          wcc_id_changed = true;
        }
      }
    }

    if (not wcc_id_changed) { break; }

    for (vertex_t u = 0; u < n; ++u) {
      if (wcc_id[u] != u) { // u is no root
        index_t r = wcc_id[u];

        // r is the root of u but maybe not a true root anymore
        // 'climb' roots till real root is found

        index_t depth = 0;
        while (wcc_id[r] != r and depth < 64) {
          r = wcc_id[r];
          depth++;
        }

        // climb once more but update
        // all entries on the way

        auto u_ = u;
        while (u_ != r) {
          auto const prev = u_;
          u_              = wcc_id[u_];
          wcc_id[prev]    = r;
        }
      }
    }
  }

  size_t wcc_count = 0;
  for (index_t u = 0; u < n; ++u) {
    if (u == wcc_id[u]) {
      // root and first occ of this wcc, as id is min in component
      // rename wcc to make the component ids continuous for easier partitioning
      wcc_id[u] = wcc_count++;
    } else {
      // rename wcc according to root
      wcc_id[u] = wcc_id[wcc_id[u]];
    }
  }

  KASPAN_STATISTIC_ADD("count", wcc_count);
  return wcc_count;
}

} // namespace kaspan
