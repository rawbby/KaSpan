#pragma once

#include <buffer/Buffer.hpp>

static void
coloring_wcc(
  I64Buffer&       color,
  I64Buffer const& sub_fw_beg,
  I64Buffer const& sub_fw_csr,
  I64Buffer const& sub_bw_beg,
  I64Buffer const& sub_bw_csr,
  i64     vert_beg,
  i64     vert_end)
{
  bool         color_changed;
  i64 depth = 0;

  do {
    depth += 1;
    color_changed = false;

    for (i64 vert_id = vert_beg; vert_id < vert_end; ++vert_id) {

      i64 my_beg = sub_bw_beg[vert_id];
      i64 my_end = sub_bw_beg[vert_id + 1];

      for (; my_beg < my_end; ++my_beg) {
        i64 const w = sub_bw_csr[my_beg];
        if (vert_id == w)
          continue;

        if (color[vert_id] < color[w]) {
          color[vert_id] = color[w];
          color_changed  = true;
        }
      }

      my_beg = sub_fw_beg[vert_id];
      my_end = sub_fw_beg[vert_id + 1];

      for (; my_beg < my_end; ++my_beg) {
        i64 const w = sub_fw_csr[my_beg];
        if (vert_id == w)
          continue;

        if (color[vert_id] < color[w]) {
          color[vert_id] = color[w];
          color_changed  = true;
        }
      }
    }

    if (color_changed) {
      for (i64 vert_id = vert_beg; vert_id < vert_end; ++vert_id) {

        if (color[vert_id] != vert_id) {
          i64 root    = color[vert_id];
          i64 c_depth = 0;
          while (color[root] != root && c_depth < 100) {
            root = color[root];
            c_depth++;
          }
          i64 v_id = vert_id;
          while (v_id != root && color[v_id] != root) {
            i64 const prev = v_id;
            v_id                    = color[v_id];
            color[prev]             = root;
          }
        }
      }
    }

  } while (color_changed);
}
