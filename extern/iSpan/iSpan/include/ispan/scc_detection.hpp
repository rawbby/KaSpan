#pragma once

#include <ispan/bw_bfs.hpp>
#include <ispan/coloring_wcc.hpp>
#include <ispan/fw_bfs.hpp>
#include <ispan/get_scc_result.hpp>
#include <ispan/gfq_origin.hpp>
#include <ispan/graph.hpp>
#include <ispan/mice_fw_bw.hpp>
#include <ispan/pivot_selection.hpp>
#include <ispan/process_wcc.hpp>
#include <ispan/trim_1_first.hpp>
#include <ispan/trim_1_normal.hpp>
#include <ispan/util.hpp>

#include <util/MpiTuple.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <vector>

inline void
scc_detection(graph const* g, int alpha, int beta, double* avg_time, int world_rank, int world_size, int run_time, std::vector<index_t>* scc_id_out = nullptr)
{
  auto const  n          = g->vert_count;
  auto const  m          = g->edge_count;
  auto const* fw_beg_pos = g->fw_beg_pos;
  auto const* fw_csr     = g->fw_csr;
  auto const* bw_beg_pos = g->bw_beg_pos;
  auto const* bw_csr     = g->bw_csr;

  auto* front_comm = new index_t[world_size]{};
  SCOPE_GUARD(delete[] front_comm);

  auto* work_comm = new int[world_size]{};
  SCOPE_GUARD(delete[] work_comm);

  auto* color_change = new bool[world_size]{};
  SCOPE_GUARD(delete[] color_change);

  index_t s = n / 32;
  if (n % 32 != 0)
    s += 1;

  index_t t = s / world_size;
  if (s % world_size != 0)
    t += 1;

  index_t step          = t * 32;
  index_t virtual_count = t * world_size * 32;
  index_t vert_beg      = std::min<index_t>(step * world_rank, n);
  index_t vert_end      = std::min<index_t>(step * (world_rank + 1), n);

  auto* sa_compress = new int[s]{};
  SCOPE_GUARD(delete[] sa_compress);

  auto* small_queue = new index_t[virtual_count]{};
  SCOPE_GUARD(delete[] small_queue);

  auto* wcc_fq = new index_t[virtual_count]{};
  SCOPE_GUARD(delete[] wcc_fq);

  auto* vert_map = new vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] vert_map);

  auto* sub_fw_beg = new vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_fw_beg);

  auto* sub_fw_csr = new vertex_t[m + 1]{};
  SCOPE_GUARD(delete[] sub_fw_csr);

  auto* sub_bw_beg = new vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_bw_beg);

  auto* sub_bw_csr = new vertex_t[m + 1]{};
  SCOPE_GUARD(delete[] sub_bw_csr);

  signed char* fw_sa;
  signed char* bw_sa;
  if (posix_memalign((void**)&fw_sa, getpagesize(), sizeof(*fw_sa) * (virtual_count + 1)))
    perror("posix_memalign");
  if (posix_memalign((void**)&bw_sa, getpagesize(), sizeof(*bw_sa) * (virtual_count + 1)))
    perror("posix_memalign");

  auto* fq_comm = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] fq_comm);

  auto* scc_id = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] scc_id);

  auto* scc_id_mice = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] scc_id_mice);

  auto* fw_sa_temp = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] fw_sa_temp);

  auto* color = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] color);

  for (int i = 0; i < virtual_count + 1; ++i) {
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

    // clang-format off

    MPI_Allgather(
      /* send: */ MPI_IN_PLACE, 0, MpiBasicType<decltype(*scc_id)>,
      /* recv: */ scc_id, step, MpiBasicType<decltype(*scc_id)>,
      /* comm: */ MPI_COMM_WORLD);

    // clang-format on

    double time_comm_trim_1 = wtime() - temp_time;
    std::cout << world_rank << ",trim-1 comm time," << time_comm_trim_1 * 1000 << ",ms\n";
    if (world_rank == 0) {
      time_size_1_first = wtime() - time;
    }
    time          = wtime();
    vertex_t root = pivot_selection(scc_id, fw_beg_pos, bw_beg_pos, 0, n, world_rank);
    pivot_time    = wtime() - time;
    time          = wtime();
    fw_bfs(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end, fw_csr, bw_csr, fw_sa, front_comm, root, world_rank, world_size, alpha, beta, m, n, step, fq_comm, sa_compress, virtual_count);
    time_fw = wtime() - time;
    for (vertex_t i = 0; i < virtual_count / 32; ++i) {
      sa_compress[i] = 0;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    time = wtime();
    bw_bfs(scc_id, fw_beg_pos, bw_beg_pos, vert_beg, vert_end, fw_csr, bw_csr, fw_sa, bw_sa, front_comm, work_comm, root, world_rank, world_size, alpha, m, n, step, fq_comm, sa_compress);
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

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, scc_id, n, MpiBasicType<decltype(*scc_id)>,
      MPI_MAX, MPI_COMM_WORLD);
    // clang-format on

    gfq_origin(n, scc_id, small_queue, fw_beg_pos, fw_csr, bw_beg_pos, bw_csr, sub_fw_beg, sub_fw_csr, sub_bw_beg, sub_bw_csr, front_comm, work_comm, world_rank, vert_map);
    vertex_t sub_v_count = front_comm[world_rank];
    if (sub_v_count > 0) {

      step = sub_v_count / world_size;
      if (sub_v_count % world_size != 0)
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
      vertex_t wcc_fq_size = 0;
      process_wcc(0, sub_v_count, wcc_fq, color, wcc_fq_size);
      if (world_rank == 0) {
        printf("color time (ms), %lf, wcc_fq, %d, time (ms), %lf\n", time_wcc * 1000, wcc_fq_size, 1000 * (wtime() - time));
      }
      mice_fw_bw(color, scc_id_mice, sub_fw_beg, sub_bw_beg, sub_fw_csr, sub_bw_csr, fw_sa_temp, world_rank, world_size, sub_v_count, wcc_fq, wcc_fq_size);
      temp_time = wtime();

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id_mice, n, MpiBasicType<decltype(*scc_id_mice)>,
        MPI_MAX, MPI_COMM_WORLD);
      // clang-format on

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

  get_scc_result(scc_id, n);
  if (scc_id_out) {
    std::memcpy(scc_id_out->data(), scc_id, std::min<size_t>(virtual_count + 1, n) * sizeof(index_t));
  }
}
