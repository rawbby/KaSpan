#pragma once

#include "util/MpiTuple.hpp"
#include "util/ScopeGuard.hpp"

#include <ispan/util.hpp>

#include <iostream>
#include <mpi.h>

inline void
bw_bfs(index_t* scc_id, int const* fw_beg_pos, int const* bw_beg_pos, index_t vert_beg, index_t vert_end, vertex_t const* fw_csr, vertex_t const* bw_csr, signed char const* fw_sa, signed char* bw_sa, index_t* front_comm, int* work_comm, vertex_t root, index_t world_rank, index_t world_size, double alpha, vertex_t edge_count, vertex_t vert_count, vertex_t step, vertex_t* fq_comm, int* sa_compress)
{
  bw_sa[root]                   = 0;
  bool        is_top_down       = true;
  bool        is_top_down_queue = false;
  signed char level             = 0;
  scc_id[root]                  = 1;
  index_t queue_size            = vert_count / 100;
  double  sync_time_bw          = 0;
  while (true) {
    double  ltm          = wtime();
    index_t front_count  = 0;
    int     my_work_next = 0;
    double  sync_time    = 0.0;
    if (is_top_down) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (bw_sa[vert_id] == level) {
          int my_beg = bw_beg_pos[vert_id];
          int my_end = bw_beg_pos[vert_id + 1];
          for (; my_beg < my_end; my_beg++) {
            vertex_t nebr = bw_csr[my_beg];
            if (bw_sa[nebr] == -1 && fw_sa[nebr] != -1) {
              bw_sa[nebr] = level + 1;
              my_work_next += bw_beg_pos[nebr + 1] - bw_beg_pos[nebr];
              fq_comm[front_count] = nebr;
              front_count++;
              scc_id[nebr] = 1;
              sa_compress[nebr / 32] |= 1 << (nebr % 32);
            }
          }
        }
      }
      work_comm[world_rank] = my_work_next;
    } else if (!is_top_down_queue) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (bw_sa[vert_id] == -1 && fw_sa[vert_id] != -1) {
          int my_beg = fw_beg_pos[vert_id];
          int my_end = fw_beg_pos[vert_id + 1];
          my_work_next += my_end - my_beg;
          for (; my_beg < my_end; my_beg++) {
            vertex_t nebr = fw_csr[my_beg];
            if (bw_sa[nebr] != -1) {
              bw_sa[vert_id]       = level + 1;
              fq_comm[front_count] = vert_id;
              front_count++;
              sa_compress[vert_id / 32] |= 1 << (vert_id % 32);
              scc_id[vert_id] = 1;
              break;
            }
          }
        }
      }
      work_comm[world_rank] = my_work_next;
    } else {

      auto* q = new index_t[queue_size]{};
      SCOPE_GUARD(delete[] q);

      index_t head = 0;
      index_t tail = 0;
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (bw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }
      while (head != tail) {
        vertex_t temp_v = q[head++];
        if (head == queue_size)
          head = 0;
        int my_beg = bw_beg_pos[temp_v];
        int my_end = bw_beg_pos[temp_v + 1];
        for (; my_beg < my_end; ++my_beg) {
          index_t w = bw_csr[my_beg];
          if (bw_sa[w] == -1 && fw_sa[w] != -1) {
            q[tail++] = w;
            if (tail == queue_size)
              tail = 0;
            scc_id[w] = 1;
            bw_sa[w]  = level + 1;
          }
        }
      }

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id, vert_count, MpiBasicType<decltype(*scc_id)>,
        MPI_MAX, MPI_COMM_WORLD);
      // clang-format on

      break;
    }
    front_comm[world_rank] = front_count;
    double temp_time       = wtime();

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, &front_count, 1, MpiBasicType<decltype(front_count)>,
      MPI_SUM, MPI_COMM_WORLD);
    // clang-format on

    if (is_top_down) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, &my_work_next, 1, MpiBasicType<decltype(my_work_next)>,
        MPI_SUM, MPI_COMM_WORLD);
      // clang-format on
    }
    if (front_count == 0) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id, vert_count, MpiBasicType<decltype(*scc_id)>,
        MPI_MAX, MPI_COMM_WORLD);
      // clang-format on

      break;
    }
    sync_time += wtime() - temp_time;
    if ((is_top_down && (my_work_next * alpha > edge_count))) {
      is_top_down = false;
      if (world_rank == 0)
        std::cout << "--->Switch to bottom up" << my_work_next << " " << edge_count << "<----\n";
    } else if (level > 50) {
      is_top_down_queue = true;

      // clang-format off

      MPI_Allgather(
        /* send: */ bw_sa + vert_beg, step, MpiBasicType<decltype(*bw_sa)>,
        /* recv: */ bw_sa, step, MpiBasicType<decltype(*bw_sa)>,
        /* comm: */ MPI_COMM_WORLD);

      // clang-format on
    }
    if (!is_top_down || is_top_down_queue) {
      double temp_time = wtime();
      if (front_count > 10000) {

        index_t s = vert_count / 32;
        if (vert_count % 32 != 0)
          s += 1;

        // clang-format off
        MPI_Allreduce(
          MPI_IN_PLACE, sa_compress, s, MpiBasicType<decltype(*sa_compress)>,
          MPI_BOR, MPI_COMM_WORLD);
        // clang-format on

        sync_time += wtime() - temp_time;
        vertex_t num_sa = 0;
        for (index_t i = 0; i < vert_count; ++i) {
          if (fw_sa[i] != -1 && bw_sa[i] == -1 && (sa_compress[i / 32] & 1 << (i % 32)) != 0) {
            bw_sa[i] = level + 1;
            num_sa += 1;
          }
        }
      } else {
        double temp_time = wtime();

        // clang-format off
        MPI_Allreduce(
          MPI_IN_PLACE, front_comm, world_size, MpiBasicType<decltype(*front_comm)>,
          MPI_MAX, MPI_COMM_WORLD);
        // clang-format on

        MPI_Request request;
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {

            // clang-format off
            MPI_Isend(
              fq_comm, front_comm[world_rank], MpiBasicType<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, &request);
            // clang-format on

            MPI_Request_free(&request);
          }
        }
        vertex_t fq_begin = front_comm[world_rank];
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {

            // clang-format off
            MPI_Recv(
              fq_comm + fq_begin, front_comm[i], MpiBasicType<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // clang-format on

            fq_begin += front_comm[i];
          }
        }
        sync_time += wtime() - temp_time;
        for (int i = 0; i < front_count; ++i) {
          vertex_t v_new = fq_comm[i];
          if (bw_sa[v_new] == -1) {
            bw_sa[v_new] = level;
          }
        }
      }
    }
    std::cout << "BW_Level-" << static_cast<int>(level) << "," << world_rank << "," << front_count << " " << sync_time * 1000 << " ms," << (wtime() - ltm) * 1000 << " ms " << "\n";
    level++;
    sync_time_bw += sync_time;
  }
  std::cout << "BW comm time," << sync_time_bw * 1000 << ",(ms)\n";
}
