#pragma once

#include <ispan/util.hpp>

inline void
trim_1_first(index_t* scc_id, int const* fw_beg_pos, int const* bw_beg_pos, index_t vert_beg, index_t vert_end)
{
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (fw_beg_pos[vert_id + 1] - fw_beg_pos[vert_id] == 0) {
      scc_id[vert_id] = -1;
    } else {
      if (bw_beg_pos[vert_id + 1] - bw_beg_pos[vert_id] == 0) {
        scc_id[vert_id] = -1;
      }
    }
  }
}