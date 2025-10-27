#pragma once

#include <util/Result.hpp>

#include <debug/Statistic.hpp>
#include <graph/AllGatherGraph.hpp>
#include <graph/Graph.hpp>
#include <util/ScopeGuard.hpp>

inline auto
wcc(Graph const& graph, U64Buffer& wcc_id) -> size_t
{
  KASPAN_STATISTIC_SCOPE("residual_wcc");

  // propagate min wcc_id along fw and bw edges until convergence
  while (true) {

    bool wcc_id_changed = false;
    auto propagate      = [&](auto u, auto v) {
      if (wcc_id[v] < wcc_id[u]) {
        wcc_id[u]      = wcc_id[v];
        wcc_id_changed = true;
      }
    };

    graph.foreach_fw_edge(propagate);
    graph.foreach_bw_edge(propagate);
    if (not wcc_id_changed)
      break;

    for (vertex_t u = 0; u < graph.n; ++u) {
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
  for (index_t u = 0; u < graph.n; ++u) {
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
