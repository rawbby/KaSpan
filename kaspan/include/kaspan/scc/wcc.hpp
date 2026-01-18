#pragma once

#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/util/scope_guard.hpp>

namespace kaspan {

inline void
wcc(
  graph_view g,
  vertex_t*  wcc_id)
{
  KASPAN_STATISTIC_SCOPE("residual_wcc");

  // propagate min wcc_id along fw and bw edges until convergence
  while (true) {

    bool wcc_id_changed = false;

    g.each_uv([&](auto u, auto v) {
      if (wcc_id[v] < wcc_id[u]) {
        wcc_id[u]      = wcc_id[v];
        wcc_id_changed = true;
      } else if (wcc_id[v] > wcc_id[u]) {
        wcc_id[v]      = wcc_id[u];
        wcc_id_changed = true;
      }
    });

    if (!wcc_id_changed) break;

    g.each_u([&](auto u) {
      if (wcc_id[u] != u) { // u is no root
        index_t r = wcc_id[u];

        // r is the root of u but maybe not a true root anymore
        // 'climb' roots till real root is found

        index_t depth = 0;
        while (wcc_id[r] != r && depth < 64) {
          r = wcc_id[r];
          ++depth;
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
    });
  }
}

} // namespace kaspan
