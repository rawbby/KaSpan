#pragma once

#include <ispan/util.hpp>

static void
coloring_wcc(index_t* color, index_t const* sub_fw_beg, index_t const* sub_fw_csr, index_t const* sub_bw_beg, index_t const* sub_bw_csr, vertex_t vert_beg, vertex_t vert_end)
{
  index_t depth = 0;
  while (true) {
    depth += 1;
    int color_changed = 0;
    for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
      index_t my_beg = sub_bw_beg[vert_id];
      index_t my_end = sub_bw_beg[vert_id + 1];
      for (; my_beg < my_end; ++my_beg) {
        index_t const w = sub_bw_csr[my_beg];
        if (vert_id == w)
          continue;
        if (color[vert_id] < color[w]) {
          color[vert_id] = color[w];
          if (color_changed == 0)
            color_changed = 1;
        }
      }
      my_beg = sub_fw_beg[vert_id];
      my_end = sub_fw_beg[vert_id + 1];
      for (; my_beg < my_end; ++my_beg) {
        index_t const w = sub_fw_csr[my_beg];
        if (vert_id == w)
          continue;
        if (color[vert_id] < color[w]) {
          color[vert_id] = color[w];
          if (color_changed == 0)
            color_changed = 1;
        }
      }
    }
    if (color_changed == 1) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
        if (color[vert_id] != vert_id) {
          index_t root    = color[vert_id];
          index_t c_depth = 0;
          while (color[root] != root && c_depth < 100) {
            root = color[root];
            c_depth++;
          }
          index_t v_id = vert_id;
          while (v_id != root && color[v_id] != root) {
            index_t const prev = v_id;
            v_id               = color[v_id];
            color[prev]        = root;
          }
        }
      }
    }
    if (color_changed == 0) {
      break;
    }
  }
}
