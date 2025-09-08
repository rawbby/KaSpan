#pragma once

#include <ispan/util.hpp>

static void
wcc_detection(index_t* wcc_id, index_t const* fw_beg, index_t const* fw_csr, index_t const* bw_beg, index_t const* bw_csr, vertex_t n)
{
  // use different internal name
  auto* color = wcc_id;

  // propagate max color along fw and bw edges until convergence

  while (true) {
    bool color_changed = false;

    for (vertex_t u = 0; u < n; ++u) {
      // bw
      {
        auto const beg = bw_beg[u];
        auto const end = bw_beg[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = bw_csr[it];

          if (color[u] < color[v]) {
            color[u]      = color[v];
            color_changed = true;
          }
        }
      }
      // fw
      {
        auto const beg = fw_beg[u];
        auto const end = fw_beg[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = fw_csr[it];

          if (color[u] < color[v]) {
            color[u]      = color[v];
            color_changed = true;
          }
        }
      }
    }

    if (not color_changed)
      break;

    for (vertex_t u = 0; u < n; ++u) {
      if (color[u] != u) { // u is no root
        index_t r = color[u];

        // r is the root of u but maybe not a true root anymore
        // 'climb' roots till real root is found

        index_t depth = 0;
        while (color[r] != r and depth < 100) {
          r = color[r];
          depth++;
        }

        // climb once more but update
        // all entries on the way

        auto u_ = u;
        while (u_ != r) {
          auto const prev = u_;
          u_              = color[u_];
          color[prev]     = r;
        }
      }
    }
  }
}
