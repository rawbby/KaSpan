#pragma once

#include <ispan/util.hpp>

static void
coloring_wcc(index_t* color, index_t const* fw_beg, index_t const* fw_csr, index_t const* bw_beg, index_t const* bw_csr, vertex_t vert_beg, vertex_t vert_end)
{
  while (true) {
    bool color_changed = false;

    for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
      // bw
      {
        auto const my_beg = bw_beg[vert_id];
        auto const my_end = bw_beg[vert_id + 1];
        for (auto my_it = my_beg; my_it < my_end; ++my_it) {
          auto const w = bw_csr[my_it];
          if (vert_id != w and color[vert_id] < color[w]) {
            color[vert_id] = color[w];
            color_changed  = true;
          }
        }
      }
      // fw
      {
        auto const my_beg = fw_beg[vert_id];
        auto const my_end = fw_beg[vert_id + 1];
        for (auto my_it = my_beg; my_it < my_end; ++my_it) {
          auto const w = fw_csr[my_it];
          if (vert_id != w and color[vert_id] < color[w]) {
            color[vert_id] = color[w];
            color_changed  = true;
          }
        }
      }
    }

    if (color_changed) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
        if (color[vert_id] != vert_id) {
          auto root  = color[vert_id];
          int  depth = 0;
          while (color[root] != root and depth < 100) {
            root = color[root];
            depth++;
          }
          auto v_id = vert_id;
          while (v_id != root && color[v_id] != root) {
            auto const prev = v_id;
            v_id            = color[v_id];
            color[prev]     = root;
          }
        }
      }
    }

    if (not color_changed)
      break;
  }
}
