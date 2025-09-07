#pragma once

#include <ispan/util.hpp>

#include <cstdio>
#include <fcntl.h>
#include <sys/types.h>

inline index_t
pivot_selection(index_t const* scc_id, int const* fw_beg_pos, int const* bw_beg_pos, index_t vert_beg, index_t vert_end, index_t world_rank)
{
  index_t max_pivot_thread  = 0;
  index_t max_degree_thread = 0;
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      int out_degree = fw_beg_pos[vert_id + 1] - fw_beg_pos[vert_id];
      int in_degree  = bw_beg_pos[vert_id + 1] - bw_beg_pos[vert_id];
      int degree_mul = out_degree * in_degree;
      if (degree_mul > max_degree_thread) {
        max_degree_thread = degree_mul;
        max_pivot_thread  = vert_id;
      }
    }
  }
  index_t max_pivot  = max_pivot_thread;
  index_t max_degree = max_degree_thread;
  {
    if (world_rank == 0)
      printf("max_pivot, %d, max_degree, %d\n", max_pivot, max_degree);
  }
  return max_pivot;
}
