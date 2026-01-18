#pragma once

#include <ispan/util.hpp>

inline void
trim_1_normal(
  kaspan::vertex_t*       scc_id,
  kaspan::index_t const*  fw_head,
  kaspan::index_t const*  bw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::vertex_t const* bw_csr,
  kaspan::vertex_t        vert_beg,
  kaspan::vertex_t        vert_end)
{
  KASPAN_STATISTIC_SCOPE("trim_1_normal");
  size_t decided_count = 0;
  for (kaspan::vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == kaspan::scc_id_undecided) {
      int             my_beg     = fw_head[vert_id];
      int             my_end     = fw_head[vert_id + 1];
      kaspan::index_t out_degree = 0;
      for (; my_beg < my_end; ++my_beg) {
        kaspan::index_t w = fw_csr[my_beg];
        if (scc_id[w] == kaspan::scc_id_undecided && w != vert_id) {
          out_degree = 1;
          break;
        }
      }
      if (out_degree == 0) {
        scc_id[vert_id] = kaspan::scc_id_singular;
        ++decided_count;
        continue;
      }
      kaspan::index_t in_degree = 0;
      my_beg                    = bw_head[vert_id];
      my_end                    = bw_head[vert_id + 1];
      for (; my_beg < my_end; ++my_beg) {
        kaspan::index_t w = bw_csr[my_beg];
        if (scc_id[w] == kaspan::scc_id_undecided && w != vert_id) {
          in_degree = 1;
          break;
        }
      }
      if (in_degree == 0) {
        scc_id[vert_id] = kaspan::scc_id_singular;
        ++decided_count;
      }
    }
  }
  KASPAN_STATISTIC_ADD("decided_count", decided_count);
}
