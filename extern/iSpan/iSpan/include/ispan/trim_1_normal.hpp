#pragma once

#include <ispan/util.hpp>

inline void
trim_1_normal(
  vertex_t*       scc_id,
  index_t const*  fw_head,
  index_t const*  bw_head,
  vertex_t const* fw_csr,
  vertex_t const* bw_csr,
  vertex_t        vert_beg,
  vertex_t        vert_end,
  size_t&         decided_count)
{
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == scc_id_undecided) {
      int     my_beg     = fw_head[vert_id];
      int     my_end     = fw_head[vert_id + 1];
      index_t out_degree = 0;
      for (; my_beg < my_end; ++my_beg) {
        index_t w = fw_csr[my_beg];
        if (scc_id[w] == scc_id_undecided && w != vert_id) {
          out_degree = 1;
          break;
        }
      }
      if (out_degree == 0) {
        scc_id[vert_id] = scc_id_singular;
        ++decided_count;
        continue;
      }
      index_t in_degree = 0;
      my_beg            = bw_head[vert_id];
      my_end            = bw_head[vert_id + 1];
      for (; my_beg < my_end; ++my_beg) {
        index_t w = bw_csr[my_beg];
        if (scc_id[w] == scc_id_undecided && w != vert_id) {
          in_degree = 1;
          break;
        }
      }
      if (in_degree == 0) {
        scc_id[vert_id] = scc_id_singular;
        ++decided_count;
      }
    }
  }
}
