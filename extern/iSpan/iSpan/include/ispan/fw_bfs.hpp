#pragma once

#include <ispan/util.hpp>

#include <iostream>
#include <mpi.h>
#include <set>

inline void
fw_bfs(index_t const* scc_id, unsigned int const* fw_beg_pos, unsigned int const* bw_beg_pos, index_t vert_beg, index_t vert_end, vertex_t const* fw_csr, vertex_t const* bw_csr, signed char* fw_sa, index_t* front_comm, vertex_t root, index_t world_rank, index_t world_size, double alpha, double beta, unsigned int edge_count, vertex_t vert_count, vertex_t step, vertex_t* fq_comm, unsigned int* sa_compress, vertex_t virtual_count)
{
  signed char level         = 0;
  fw_sa[root]               = 0;
  bool    is_top_down       = true;
  bool    is_top_down_queue = false;
  index_t queue_size        = vert_count / 100;
  double  sync_time_fw      = 0;
  while (true) {
    double const ltm          = wtime();
    double       sync_time    = 0.0;
    index_t      front_count  = 0;
    unsigned int my_work_next = 0;
    if (is_top_down) {
      std::cout << vert_beg << "," << vert_end << "\n";
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == (level)) {
          unsigned int       my_beg = fw_beg_pos[vert_id];
          unsigned int const my_end = fw_beg_pos[vert_id + 1];
          for (; my_beg < my_end; my_beg++) {
            vertex_t const nebr = fw_csr[my_beg];
            if (scc_id[nebr] == 0 && fw_sa[nebr] == -1) {
              fw_sa[nebr] = level + 1;
              sa_compress[nebr / 32] |= 1 << (static_cast<index_t>(nebr) % 32);
              my_work_next += fw_beg_pos[nebr + 1] - fw_beg_pos[nebr];
              fq_comm[front_count] = nebr;
              front_count++;
            }
          }
        }
      }
      std::cout << my_work_next << "\n";
    } else if (!is_top_down_queue) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == -1) {
          unsigned int       my_beg = bw_beg_pos[vert_id];
          unsigned int const my_end = bw_beg_pos[vert_id + 1];
          my_work_next += my_end - my_beg;
          for (; my_beg < my_end; my_beg++) {
            vertex_t const nebr = bw_csr[my_beg];
            if (scc_id[vert_id] == 0 && fw_sa[nebr] != -1) {
              fw_sa[vert_id]       = level + 1;
              fq_comm[front_count] = vert_id;
              sa_compress[vert_id / 32] |= ((index_t)1 << ((index_t)vert_id % 32));
              front_count++;
              break;
            }
          }
        }
      }
    } else {
      auto*   q    = new index_t[queue_size];
      index_t head = 0;
      index_t tail = 0;
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }
      while (head != tail) {
        vertex_t const temp_v = q[head++];
        if (head == queue_size)
          head = 0;
        unsigned int       my_beg = fw_beg_pos[temp_v];
        unsigned int const my_end = fw_beg_pos[temp_v + 1];
        for (; my_beg < my_end; ++my_beg) {
          index_t w = fw_csr[my_beg];
          if (scc_id[w] == 0 && fw_sa[w] == -1) {
            q[tail++] = w;
            if (tail == queue_size)
              tail = 0;
            fw_sa[w] = level + 1;
          }
        }
      }
      MPI_Allreduce(MPI_IN_PLACE, fw_sa, vert_count, MPI_SIGNED_CHAR, MPI_MAX, MPI_COMM_WORLD);
      break;
    }
    front_comm[world_rank] = front_count;
    double const temp_time = wtime();
    if (is_top_down) {
      MPI_Allreduce(MPI_IN_PLACE, &my_work_next, 1, MPI_UNSIGNED, MPI_SUM, MPI_COMM_WORLD);
    }
    MPI_Allreduce(MPI_IN_PLACE, &front_count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    sync_time += wtime() - temp_time;
    if (front_count == 0) {
      double temp_time = wtime();
      MPI_Allreduce(MPI_IN_PLACE, fw_sa, vert_count, MPI_SIGNED_CHAR, MPI_MAX, MPI_COMM_WORLD);
      std::cout << "FW final sync," << static_cast<int>(level) << ",world_rank" << world_rank << ",time," << (wtime() - temp_time) * 1000 << " ms\n";
      break;
    }
    if (is_top_down && my_work_next * alpha > edge_count) {
      is_top_down = false;
      if (world_rank == 0)
        std::cout << "--->Switch to bottom-up, level = " << static_cast<int>(level) << "\n";
    } else if (level > 50) {
      is_top_down_queue = true;
      double temp_time  = wtime();
      MPI_Allgather(&fw_sa[vert_beg], step, MPI_SIGNED_CHAR, fw_sa, step, MPI_SIGNED_CHAR, MPI_COMM_WORLD);
      if (world_rank == 0)
        std::cout << "--->Switch to async top-down, level = " << static_cast<int>(level) << "<----\n";
      sync_time += wtime() - temp_time;
    }
    if (is_top_down_queue || !is_top_down) {
      double temp_time = wtime();
      if (front_count > 10000) {
        double temp_time = wtime();
        MPI_Allreduce(MPI_IN_PLACE, sa_compress, virtual_count / 32, MPI_UNSIGNED, MPI_BOR, MPI_COMM_WORLD);
        sync_time += wtime() - temp_time;
        vertex_t num_sa = 0;
        for (index_t i = 0; i < vert_count; ++i) {
          if (scc_id[i] == 0 && fw_sa[i] == -1 && ((sa_compress[i / 32] & ((index_t)1 << ((index_t)i % 32))) != 0)) {
            fw_sa[i] = level + 1;
            num_sa += 1;
          }
        }
      } else {
        MPI_Allreduce(MPI_IN_PLACE, front_comm, world_size, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Request request;
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {
            MPI_Isend(fq_comm, front_comm[world_rank], MPI_INT, i, 0, MPI_COMM_WORLD, &request);
            MPI_Request_free(&request);
          }
        }
        vertex_t fq_begin = front_comm[world_rank];
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {
            MPI_Recv(&fq_comm[fq_begin], front_comm[i], MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            fq_begin += front_comm[i];
          }
        }
        sync_time += wtime() - temp_time;
        for (int i = front_comm[world_rank]; i < front_count; ++i) {
          vertex_t const v_new = fq_comm[i];
          if (fw_sa[v_new] == -1) {
            fw_sa[v_new] = level + 1;
          }
        }
      }
    }
    if (world_rank == 0)
      std::cout << "FW_Level-" << static_cast<int>(level) << "," << world_rank << "," << front_count << " " << sync_time * 1000 << " ms," << (wtime() - ltm) * 1000 << " ms " << "\n";
    level++;
    sync_time_fw += sync_time;
  }
  std::cout << "FW comm time," << sync_time_fw * 1000 << ",(ms)\n";
}