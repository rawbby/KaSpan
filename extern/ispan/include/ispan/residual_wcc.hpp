#pragma once

#include <ispan/util.hpp>

static void
wcc(
  kaspan::vertex_t*      wcc_id,
  kaspan::index_t const* fw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::index_t const* bw_head,
  kaspan::vertex_t const* bw_csr,
  kaspan::vertex_t       n)
{
  // use different internal name
  auto* color = wcc_id;

  // propagate max color along fw and bw edges until convergence

  while (true) {
    bool color_changed = false;

    for (kaspan::vertex_t u = 0; u < n; ++u) {
      // bw
      {
        auto const beg = bw_head[u];
        auto const end = bw_head[u + 1];
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
        auto const beg = fw_head[u];
        auto const end = fw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = fw_csr[it];

          if (color[u] < color[v]) {
            color[u]      = color[v];
            color_changed = true;
          }
        }
      }
    }

    if (!color_changed) break;

    for (kaspan::vertex_t u = 0; u < n; ++u) {
      if (color[u] != u) { // u is no root
        kaspan::index_t r = color[u];

        // r is the root of u but maybe not a true root anymore
        // 'climb' roots till real root is found

        kaspan::index_t depth = 0;
        while (color[r] != r && depth < 100) {
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
