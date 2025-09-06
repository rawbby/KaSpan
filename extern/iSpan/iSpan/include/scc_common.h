#pragma once

#include "fw_bw.h"
#include "graph.h"
#include "wtime.h"
#include "color_propagation.h"
#include "trim_1_gfq.h"
#include "util.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mpi.h>
#include <sys/types.h>
#include <unistd.h>

inline index_t
pivot_selection(index_t const* scc_id, unsigned int const* fw_beg_pos, unsigned int const* bw_beg_pos, index_t vert_beg, index_t vert_end, index_t world_rank)
{
  index_t max_pivot_thread  = 0;
  index_t max_degree_thread = 0;
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      unsigned int out_degree = fw_beg_pos[vert_id + 1] - fw_beg_pos[vert_id];
      unsigned int in_degree  = bw_beg_pos[vert_id + 1] - bw_beg_pos[vert_id];
      unsigned int degree_mul = out_degree * in_degree;
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

inline void
scc_detection(graph const* g, int alpha, int beta, double* avg_time, int world_rank, int world_size, int run_time)
{
  index_t const vert_count   = g->vert_count;
  unsigned int const  edge_count   = g->edge_count;
  unsigned int*       fw_beg_pos   = g->fw_beg_pos;
  vertex_t*     fw_csr       = g->fw_csr;
  unsigned int*       bw_beg_pos   = g->bw_beg_pos;
  vertex_t*     bw_csr       = g->bw_csr;
  auto*         front_comm   = static_cast<index_t*>(calloc(world_size, sizeof(index_t)));
  auto*         work_comm    = static_cast<unsigned int*>(calloc(world_size, sizeof(unsigned int)));
  auto*         color_change = new bool[world_size];
  memset(color_change, 0, sizeof(bool) * world_size);
  vertex_t      wcc_fq_size = 0;
  index_t const p_count     = world_size;
  index_t       s           = vert_count / 32;
  if (vert_count % 32 != 0)
    s += 1;
  index_t t = s / p_count;
  if (s % p_count != 0)
    t += 1;
  index_t  step          = t * 32;
  index_t  virtual_count = t * p_count * 32;
  index_t  vert_beg      = world_rank * step;
  index_t  vert_end      = world_rank == p_count - 1 ? vert_count : vert_beg + step;
  auto*    sa_compress   = static_cast<unsigned int*>(calloc(s, sizeof(unsigned int)));
  auto*    small_queue   = new index_t[virtual_count + 1];
  auto*    wcc_fq        = new index_t[virtual_count + 1];
  auto*    vert_map      = static_cast<vertex_t*>(calloc(vert_count + 1, sizeof(vertex_t)));
  auto*    sub_fw_beg    = static_cast<vertex_t*>(calloc(vert_count + 1, sizeof(vertex_t)));
  auto*    sub_fw_csr    = static_cast<vertex_t*>(calloc(edge_count + 1, sizeof(vertex_t)));
  auto*    sub_bw_beg    = static_cast<vertex_t*>(calloc(vert_count + 1, sizeof(vertex_t)));
  auto*    sub_bw_csr    = static_cast<vertex_t*>(calloc(edge_count + 1, sizeof(vertex_t)));
  signed char* fw_sa;
  signed char* bw_sa;
  if (posix_memalign((void**)&fw_sa, getpagesize(), sizeof(signed char) * (virtual_count + 1)))
    perror("posix_memalign");
  if (posix_memalign((void**)&bw_sa, getpagesize(), sizeof(signed char) * (virtual_count + 1)))
    perror("posix_memalign");
  auto* fq_comm     = static_cast<vertex_t*>(calloc(virtual_count + 1, sizeof(vertex_t)));
  auto* scc_id      = new vertex_t[virtual_count + 1];
  auto* scc_id_mice = static_cast<vertex_t*>(calloc(virtual_count + 1, sizeof(vertex_t)));
  auto* fw_sa_temp  = static_cast<vertex_t*>(calloc(virtual_count + 1, sizeof(vertex_t)));
  auto* color       = static_cast<vertex_t*>(calloc(virtual_count + 1, sizeof(vertex_t)));
  for (unsigned int i = 0; i < virtual_count + 1; ++i) {
    fw_sa[i]       = -1;
    bw_sa[i]       = -1;
    scc_id[i]      = 0;
    scc_id_mice[i] = -1;
  }
  double const start_time = wtime();
  double       end_time;
  {
    double time_size_1_first;
    double time_size_1;
    double time_fw;
    double time_bw;
    double time_size_2;
    double time_size_3;
    double time_gfq;
    double pivot_time;
    double time_wcc = 0.0;
    double time_mice_fw_bw;
    double time_comm;
    MPI_Barrier(MPI_COMM_WORLD);
    double time = wtime();
    trim_1_first(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end);
    std::cout << world_rank << ",Computing size_1_first cost," << (wtime() - time) * 1000 << " ms\n";
    double temp_time = wtime();
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, scc_id, step, MPI_INT, MPI_COMM_WORLD);
    double time_comm_trim_1 = wtime() - temp_time;
    std::cout << world_rank << ",trim-1 comm time," << time_comm_trim_1 * 1000 << ",ms\n";
    if (world_rank == 0) {
      time_size_1_first = wtime() - time;
    }
    time          = wtime();
    vertex_t root = pivot_selection(scc_id, fw_beg_pos, bw_beg_pos, 0, vert_count, world_rank);
    pivot_time    = wtime() - time;
    time          = wtime();
    fw_bfs(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end, fw_csr, bw_csr, fw_sa, front_comm, root, world_rank, world_size, alpha, beta, edge_count, vert_count, step, fq_comm, sa_compress, virtual_count);
    time_fw = wtime() - time;
    for (vertex_t i = 0; i < virtual_count / 32; ++i) {
      sa_compress[i] = 0;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    time = wtime();
    bw_bfs(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end, fw_csr, bw_csr, fw_sa, bw_sa, front_comm, work_comm, root, world_rank, world_size, alpha, edge_count, vert_count, step, fq_comm, sa_compress);
    time_bw = wtime() - time;
    printf("bw_bfs done\n");
    time = wtime();
    trim_1_normal(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end, fw_csr, bw_csr);
    if (world_rank == 0) {
      time_size_1 += wtime() - time;
    }
    time = wtime();
    trim_1_normal(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end, fw_csr, bw_csr);
    time_size_1 += wtime() - time;
    time = wtime();
    MPI_Allreduce(MPI_IN_PLACE, scc_id, vert_count, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    gfq_origin(vert_count, scc_id, small_queue, fw_beg_pos, fw_csr, bw_beg_pos, bw_csr, sub_fw_beg, sub_fw_csr, sub_bw_beg, sub_bw_csr, front_comm, work_comm, world_rank, vert_map);
    vertex_t sub_v_count = front_comm[world_rank];
    if (sub_v_count > 0) {
      step = sub_v_count / p_count;
      if (sub_v_count % p_count != 0)
        step += 1;
      vert_beg = world_rank * step;
      for (index_t i = 0; i < sub_v_count; ++i) {
        color[i] = i;
      }
      time = wtime();
      coloring_wcc(color, sub_fw_beg, sub_fw_csr, sub_bw_beg, sub_bw_csr, 0, sub_v_count);
      if (world_rank == 0) {
        time_wcc += wtime() - time;
      }
      time = wtime();
      process_wcc(0, sub_v_count, wcc_fq, color, wcc_fq_size);
      if (world_rank == 0) {
        printf("color time (ms), %lf, wcc_fq, %d, time (ms), %lf\n", time_wcc * 1000, wcc_fq_size, 1000 * (wtime() - time));
      }
      mice_fw_bw(color, scc_id_mice, sub_fw_beg, sub_bw_beg, sub_fw_csr, sub_bw_csr, fw_sa_temp, world_rank, world_size, sub_v_count, wcc_fq, wcc_fq_size);
      temp_time = wtime();
      MPI_Allreduce(MPI_IN_PLACE, scc_id_mice, vert_count, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
      time_comm = wtime() - temp_time;
      printf("%d,final comm time,%.3lf\n", world_rank, time_comm * 1000);
      for (int i = 0; i < sub_v_count; ++i) {
        vertex_t actual_v = small_queue[i];
        scc_id[actual_v]  = small_queue[scc_id_mice[i]];
      }
    }
    if (world_rank == 0) {
      time_mice_fw_bw = wtime() - time;
    }
    if (world_rank == 0 && run_time != 1) {
      avg_time[0] += time_size_1_first + time_size_1 + time_size_2 + time_size_3;
      avg_time[1] += time_fw + time_bw;
      avg_time[2] += time_wcc + time_mice_fw_bw;
      avg_time[4] += time_size_1_first + time_size_1;
      avg_time[5] += time_size_2;
      avg_time[6] += pivot_time;
      avg_time[7] += time_fw;
      avg_time[8] += time_bw;
      avg_time[9] += time_wcc;
      avg_time[10] += time_mice_fw_bw;
      avg_time[13] += time_size_3;
      avg_time[14] += time_gfq;
    }
  }
  end_time = wtime() - start_time;
  avg_time[3] += end_time;
  get_scc_result(scc_id, vert_count);
}
