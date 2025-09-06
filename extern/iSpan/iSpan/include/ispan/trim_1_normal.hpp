#pragma once

#include <ispan/util.hpp>

inline void
trim_1_normal(index_t* scc_id, unsigned int const* fw_beg_pos, unsigned int const* bw_beg_pos, index_t vert_beg, index_t vert_end, vertex_t const* fw_csr, vertex_t const* bw_csr)
{
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      unsigned int my_beg     = fw_beg_pos[vert_id];
      unsigned int my_end     = fw_beg_pos[vert_id + 1];
      index_t      out_degree = 0;
      for (; my_beg < my_end; ++my_beg) {
        index_t w = fw_csr[my_beg];
        if (scc_id[w] == 0 && w != vert_id) {
          out_degree = 1;
          break;
        }
      }
      if (out_degree == 0) {
        scc_id[vert_id] = -1;
        continue;
      }
      index_t in_degree = 0;
      my_beg            = bw_beg_pos[vert_id];
      my_end            = bw_beg_pos[vert_id + 1];
      for (; my_beg < my_end; ++my_beg) {
        index_t w = bw_csr[my_beg];
        if (scc_id[w] == 0 && w != vert_id) {
          in_degree = 1;
          break;
        }
      }
      if (in_degree == 0) {
        scc_id[vert_id] = -1;
      }
    }
  }
}