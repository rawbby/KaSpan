#pragma once

#include "../../src/util.h"
#include <vector>

static void
coloring_wcc(
  std::vector<index_t>& color,
  const std::vector<index_t>& sub_fw_beg,
  const std::vector<index_t>& sub_fw_csr,
  const std::vector<index_t>& sub_bw_beg,
  const std::vector<index_t>& sub_bw_csr,
  vertex_t vert_beg,
  vertex_t vert_end)
{
  bool color_changed;
  index_t depth = 0;

  do {
    depth += 1;
    color_changed = false;

    for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {

      index_t my_beg = sub_bw_beg[vert_id];
      index_t my_end = sub_bw_beg[vert_id + 1];

      for (; my_beg < my_end; ++my_beg) {
        const index_t w = sub_bw_csr[my_beg];
        if (vert_id == w)
          continue;

        if (color[vert_id] < color[w]) {
          color[vert_id] = color[w];
          color_changed = true;
        }
      }

      my_beg = sub_fw_beg[vert_id];
      my_end = sub_fw_beg[vert_id + 1];

      for (; my_beg < my_end; ++my_beg) {
        const index_t w = sub_fw_csr[my_beg];
        if (vert_id == w)
          continue;

        if (color[vert_id] < color[w]) {
          color[vert_id] = color[w];
          color_changed = true;
        }
      }
    }

    if (color_changed) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {

        if (color[vert_id] != vert_id) {
          index_t root = color[vert_id];
          index_t c_depth = 0;
          while (color[root] != root && c_depth < 100) {
            root = color[root];
            c_depth++;
          }
          index_t v_id = vert_id;
          while (v_id != root && color[v_id] != root) {
            const index_t prev = v_id;
            v_id = color[v_id];
            color[prev] = root;
          }
        }
      }
    }

  } while (color_changed);
}
