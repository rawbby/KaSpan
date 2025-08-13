#pragma once


#include "util.h"
#include "wtime.h"

#include <iostream>
#include <mpi.h>
#include <set>
#include <vector>

inline void
fw_bfs(
  const std::vector<index_t>& scc_id,
  const std::vector<long_t>& fw_beg_pos,
  const std::vector<long_t>& bw_beg_pos,
  index_t vert_beg,
  index_t vert_end,
  const std::vector<vertex_t>& fw_csr,
  const std::vector<vertex_t>& bw_csr,
  depth_t* fw_sa,
  std::vector<index_t>& front_comm,
  vertex_t root,
  index_t tid,
  double alpha,
  long_t edge_count,
  vertex_t vert_count,
  vertex_t world_size,
  vertex_t world_rank,
  vertex_t step,
  std::vector<vertex_t>& fq_comm,
  std::vector<unsigned int>& sa_compress,
  vertex_t virtual_count)
{
  depth_t level = 0;

  fw_sa[root] = 0;
  bool is_top_down = true;
  bool is_top_down_queue = false;
  index_t queue_size = vert_count / 100;

  double sync_time_fw = 0;
  while (true) {
    const auto ltm = wtime();
    double sync_time = 0.0;
    index_t front_count = 0;
    long_t my_work_next = 0;

    if (is_top_down) {
      std::cout << vert_beg << "," << vert_end << "\n";
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == (level)) {

          const auto my_end = fw_beg_pos[vert_id + 1];
          for (auto my_beg = fw_beg_pos[vert_id]; my_beg < my_end; my_beg++) {
            const vertex_t nebr = fw_csr[my_beg];
            if (scc_id[nebr] == 0 && fw_sa[nebr] == -1) {
              fw_sa[nebr] = level + 1;
              sa_compress[nebr / 32] |= static_cast<index_t>(1) << nebr % 32;

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
          auto my_beg = bw_beg_pos[vert_id];
          const auto my_end = bw_beg_pos[vert_id + 1];
          my_work_next += my_end - my_beg;

          for (; my_beg < my_end; my_beg++) {
            const auto nebr = bw_csr[my_beg];
            if (scc_id[vert_id] == 0 && fw_sa[nebr] != -1) {
              fw_sa[vert_id] = level + 1;
              fq_comm[front_count] = vert_id;
              sa_compress[vert_id / 32] |= static_cast<index_t>(1) << vert_id % 32;
              front_count++;
              break;
            }
          }
        }
      }
    } else {
      std::vector<index_t> q(queue_size);
      index_t head = 0;
      index_t tail = 0;

      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }

      while (head != tail) {
        vertex_t temp_v = q[head++];
        if (head == queue_size)
          head = 0;

        const auto my_end = fw_beg_pos[temp_v + 1];
        for (auto my_beg = fw_beg_pos[temp_v]; my_beg < my_end; ++my_beg) {
          const index_t w = fw_csr[my_beg];

          if (scc_id[w] == 0 && fw_sa[w] == -1) {
            q[tail++] = w;
            if (tail == queue_size)
              tail = 0;
            fw_sa[w] = level + 1;
          }
        }
      }

      const double temp_time = wtime();
      MPI_Allreduce(MPI_IN_PLACE,
                    fw_sa,
                    vert_count,
                    MPI_SIGNED_CHAR,
                    MPI_MAX,
                    MPI_COMM_WORLD);
      sync_time += wtime() - temp_time;

      break;
    }

    front_comm[tid] = front_count;

    const auto temp_time = wtime();

    if (is_top_down) {
      MPI_Allreduce(MPI_IN_PLACE,
                    &my_work_next,
                    1,
                    MPI_LONG,
                    MPI_SUM,
                    MPI_COMM_WORLD);
    }

    MPI_Allreduce(MPI_IN_PLACE,
                  &front_count,
                  1,
                  MPI_INT,
                  MPI_SUM,
                  MPI_COMM_WORLD);

    sync_time += wtime() - temp_time;

    if (front_count == 0) {
      const auto temp_time = wtime();
      MPI_Allreduce(MPI_IN_PLACE,
                    fw_sa,
                    vert_count,
                    MPI_SIGNED_CHAR,
                    MPI_MAX,
                    MPI_COMM_WORLD);
      std::cout << "FW final sync," << (int)level << ",tid" << tid << ",time," << (wtime() - temp_time) * 1000 << " ms\n";
      break;
    }

    if ((is_top_down && (my_work_next * alpha > edge_count))) {
      is_top_down = false;

      if (tid == 0)

        std::cout << "--->Switch to bottom-up, level = " << (int)(level) << "\n";
    } else if (level > 50) {
      is_top_down_queue = true;
      const auto temp_time = wtime();

      MPI_Allgather(&fw_sa[vert_beg],
                    step,
                    MPI_SIGNED_CHAR,
                    fw_sa,
                    step,
                    MPI_SIGNED_CHAR,
                    MPI_COMM_WORLD);

      if (tid == 0)
        std::cout << "--->Switch to async top-down, level = " << static_cast<int>(level) << "<----\n";

      sync_time += wtime() - temp_time;
    }

    if (is_top_down_queue || !is_top_down) {
      const auto temp_time = wtime();

      if (front_count > 10000) {
        const auto temp_time = wtime();

        MPI_Allreduce(MPI_IN_PLACE,
                      sa_compress.data(),
                      virtual_count / 32,
                      MPI_UNSIGNED,
                      MPI_BOR,
                      MPI_COMM_WORLD);
        sync_time += wtime() - temp_time;

        vertex_t num_sa = 0;
        for (index_t i = 0; i < vert_count; ++i) {
          if (scc_id[i] == 0 && fw_sa[i] == -1 && (sa_compress[i / 32] & static_cast<index_t>(1) << (i % 32)) != 0) {
            fw_sa[i] = level + 1;
            num_sa += 1;
          }
        }
      } else {
        MPI_Allreduce(MPI_IN_PLACE,
                      front_comm.data(),
                      world_size,
                      MPI_INT,
                      MPI_MAX,
                      MPI_COMM_WORLD);

        MPI_Request request;
        for (vertex_t i = 0; i < world_size; ++i) {
          if (i != world_rank) {
            MPI_Isend(fq_comm.data(),
                      front_comm[world_rank],
                      MPI_INT,
                      i,
                      0,
                      MPI_COMM_WORLD,
                      &request);
            MPI_Request_free(&request);
          }
        }

        auto fq_begin = front_comm[world_rank];
        for (vertex_t i = 0; i < world_size; ++i) {
          if (i != world_rank) {
            MPI_Recv(&fq_comm[fq_begin],
                     front_comm[i],
                     MPI_INT,
                     i,
                     0,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            fq_begin += front_comm[i];
          }
        }
        sync_time += wtime() - temp_time;

        for (auto i = front_comm[world_rank]; i < front_count; ++i) {
          const auto v_new = fq_comm[i];
          if (fw_sa[v_new] == -1) {
            fw_sa[v_new] = level + 1;
          }
        }
      }
    }

    if (tid == 0)
      std::cout << "FW_Level-" << static_cast<int>(level)
                << "," << tid << ","
                << front_count << " "
                << sync_time * 1000 << " ms,"
                << (wtime() - ltm) * 1000 << " ms "
                << "\n";

    level++;
    sync_time_fw += sync_time;
  }
  std::cout << "FW comm time," << sync_time_fw * 1000 << ",(ms)\n";
}

inline void
bw_bfs(
  std::vector<index_t>& scc_id,
  const std::vector<long_t>& fw_beg_pos,
  const std::vector<long_t>& bw_beg_pos,
  index_t vert_beg,
  index_t vert_end,
  const std::vector<vertex_t>& fw_csr,
  const std::vector<vertex_t>& bw_csr,
  const depth_t* fw_sa,
  depth_t* bw_sa,
  std::vector<index_t>& front_comm,
  std::vector<long_t>& work_comm,
  vertex_t root,
  index_t tid,
  double alpha,
  vertex_t edge_count,
  vertex_t vert_count,
  vertex_t world_size,
  vertex_t world_rank,
  vertex_t step,
  std::vector<vertex_t>& fq_comm,
  std::vector<unsigned int>& sa_compress)
{
  bw_sa[root] = 0;
  bool is_top_down = true;
  bool is_top_down_queue = false;
  depth_t level = 0;
  scc_id[root] = 1;
  index_t queue_size = vert_count / 100;

  double sync_time_bw = 0;

  while (true) {
    double ltm = wtime();
    index_t front_count = 0;
    long_t my_work_next = 0;

    double sync_time = 0.0;
    if (is_top_down) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (bw_sa[vert_id] == level) {
          long_t my_beg = bw_beg_pos[vert_id];
          long_t my_end = bw_beg_pos[vert_id + 1];

          for (; my_beg < my_end; my_beg++) {
            vertex_t nebr = bw_csr[my_beg];

            if (bw_sa[nebr] == -1 && fw_sa[nebr] != -1) {
              bw_sa[nebr] = level + 1;
              my_work_next += bw_beg_pos[nebr + 1] - bw_beg_pos[nebr];
              fq_comm[front_count] = nebr;
              front_count++;
              scc_id[nebr] = 1;
              sa_compress[nebr / 32] |= ((index_t)1 << ((index_t)nebr % 32));
            }
          }
        }
      }
      work_comm[tid] = my_work_next;
    } else if (!is_top_down_queue) {
      for (vertex_t vert_id = vert_beg; vert_id < vert_end; vert_id++) {
        if (bw_sa[vert_id] == -1 && fw_sa[vert_id] != -1) {
          long_t my_beg = fw_beg_pos[vert_id];
          long_t my_end = fw_beg_pos[vert_id + 1];
          my_work_next += my_end - my_beg;

          for (; my_beg < my_end; my_beg++) {
            vertex_t nebr = fw_csr[my_beg];

            if (bw_sa[nebr] != -1) {
              bw_sa[vert_id] = level + 1;
              fq_comm[front_count] = vert_id;
              front_count++;
              sa_compress[vert_id / 32] |= ((index_t)1 << ((index_t)vert_id % 32));
              scc_id[vert_id] = 1;
              break;
            }
          }
        }
      }
      work_comm[tid] = my_work_next;
    } else {
      std::vector<index_t> q(queue_size);
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
        long_t my_beg = bw_beg_pos[temp_v];
        long_t my_end = bw_beg_pos[temp_v + 1];

        for (; my_beg < my_end; ++my_beg) {
          index_t w = bw_csr[my_beg];

          if (bw_sa[w] == -1 && fw_sa[w] != -1) {
            q[tail++] = w;
            if (tail == queue_size)
              tail = 0;
            scc_id[w] = 1;
            bw_sa[w] = level + 1;
          }
        }
      }

      MPI_Allreduce(MPI_IN_PLACE,
                    scc_id.data(),
                    scc_id.size(),
                    MPI_LONG,
                    MPI_MAX,
                    MPI_COMM_WORLD);

      break;
    }

    front_comm[tid] = front_count;

    double temp_time = wtime();
    MPI_Allreduce(MPI_IN_PLACE,
                  &front_count,
                  1,
                  MPI_INT,
                  MPI_SUM,
                  MPI_COMM_WORLD);
    if (is_top_down) {
      MPI_Allreduce(MPI_IN_PLACE,
                    &my_work_next,
                    1,
                    MPI_INT,
                    MPI_SUM,
                    MPI_COMM_WORLD);
    }

    if (front_count == 0) {
      MPI_Allreduce(MPI_IN_PLACE,
                    scc_id.data(),
                    scc_id.size(),
                    MPI_LONG,
                    MPI_MAX,
                    MPI_COMM_WORLD);
      break;
    }
    sync_time += wtime() - temp_time;

    if ((is_top_down && (my_work_next * alpha > edge_count))) {
      is_top_down = false;

      if (tid == 0)
        std::cout << "--->Switch to bottom up" << my_work_next
                  << " " << edge_count << "<----\n";
    } else if (level > 50) {
      is_top_down_queue = true;

      MPI_Allgather(&bw_sa[vert_beg],
                    step,
                    MPI_SIGNED_CHAR,
                    bw_sa,
                    step,
                    MPI_SIGNED_CHAR,
                    MPI_COMM_WORLD);
    }

    if (!is_top_down || is_top_down_queue) {
      const auto temp_time = wtime();
      if (front_count > 10000) {
        MPI_Allreduce(MPI_IN_PLACE,
                      sa_compress.data(),
                      sa_compress.size(),
                      MPI_UNSIGNED,
                      MPI_BOR,
                      MPI_COMM_WORLD);
        sync_time += wtime() - temp_time;

        vertex_t num_sa = 0;
        for (index_t i = 0; i < vert_count; ++i) {
          if (fw_sa[i] != -1 && bw_sa[i] == -1 && ((sa_compress[i / 32] & ((index_t)1 << ((index_t)i % 32))) != 0)) {
            bw_sa[i] = level + 1;
            num_sa += 1;
          }
        }
      } else {
        const auto local_temp_time = wtime();

        MPI_Allreduce(MPI_IN_PLACE,
                      front_comm.data(),
                      front_comm.size(),
                      MPI_INT,
                      MPI_MAX,
                      MPI_COMM_WORLD);

        MPI_Request request;

        for (vertex_t i = 0; i < world_size; ++i) {
          if (i != world_rank) {
            MPI_Isend(fq_comm.data(),
                      front_comm[world_rank],
                      MPI_INT,
                      i,
                      0,
                      MPI_COMM_WORLD,
                      &request);
            MPI_Request_free(&request);
          }
        }

        vertex_t fq_begin = front_comm[world_rank];
        for (vertex_t i = 0; i < world_size; ++i) {
          if (i != world_rank) {
            MPI_Recv(&fq_comm[fq_begin],
                     front_comm[i],
                     MPI_INT,
                     i,
                     0,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            fq_begin += front_comm[i];
          }
        }

        sync_time += wtime() - local_temp_time;

        for (index_t i = 0; i < front_count; ++i) {
          vertex_t v_new = fq_comm[i];
          if (bw_sa[v_new] == -1) {
            bw_sa[v_new] = level;
          }
        }
      }
    }

    std::cout << "BW_Level-" << (int)level
              << "," << tid << ","
              << front_count << " "
              << sync_time * 1000 << " ms,"
              << (wtime() - ltm) * 1000 << " ms "
              << "\n";

    level++;
    sync_time_bw += sync_time;
  }
  std::cout << "BW comm time," << sync_time_bw * 1000 << ",(ms)\n";
}

inline void
fw_bfs_fq(
  const index_t* scc_id,
  const index_t* fw_beg_pos,
  const index_t* bw_beg_pos,
  index_t vert_beg,
  index_t vert_end,
  const vertex_t* fw_csr,
  const vertex_t* bw_csr,
  vertex_t* fw_sa,
  index_t* vertex_front,
  vertex_t root,
  index_t tid,
  index_t thread_count,
  double alpha,
  double beta,
  const vertex_t* frontier_queue,
  vertex_t fq_size,
  const double avg_degree,
  vertex_t vertex_visited)
{
  depth_t level = 0;
  fw_sa[root] = 0;
  const vertex_t root_out_degree = fw_beg_pos[root + 1] - fw_beg_pos[root];
  bool is_top_down = true;
  bool is_top_down_async = false;
  if (root_out_degree < alpha * beta * fq_size) {
    is_top_down_async = true;
  }
  bool is_top_down_queue = false;
  const index_t queue_size = fq_size / thread_count;
#pragma omp barrier

  while (true) {
    vertex_t vertex_frontier = 0;

    if (is_top_down) {
      if (is_top_down_async) {
        for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
          const vertex_t vert_id = frontier_queue[fq_vert_id];

          if (scc_id[vert_id] == 0 && (fw_sa[vert_id] == level || fw_sa[vert_id] == level + 1)) {
            index_t my_beg = fw_beg_pos[vert_id];
            const index_t my_end = fw_beg_pos[vert_id + 1];

            for (; my_beg < my_end; my_beg++) {
              const vertex_t nebr = fw_csr[my_beg];
              if (scc_id[nebr] == 0 && fw_sa[nebr] == -1) {
                fw_sa[nebr] = level + 1;

                vertex_frontier++;
              }
            }
          }
        }
      } else {
        for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
          const vertex_t vert_id = frontier_queue[fq_vert_id];

          if (scc_id[vert_id] == 0 && fw_sa[vert_id] == level) {
            index_t my_beg = fw_beg_pos[vert_id];
            const index_t my_end = fw_beg_pos[vert_id + 1];

            for (; my_beg < my_end; my_beg++) {
              vertex_t nebr = fw_csr[my_beg];
              if (scc_id[nebr] == 0 && fw_sa[nebr] == -1) {
                fw_sa[nebr] = level + 1;

                vertex_frontier++;
              }
            }
          }
        }
      }
    } else if (!is_top_down_queue) {
      for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
        vertex_t vert_id = frontier_queue[fq_vert_id];
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == -1) {
          index_t my_beg = bw_beg_pos[vert_id];
          index_t my_end = bw_beg_pos[vert_id + 1];

          for (; my_beg < my_end; my_beg++) {
            vertex_t nebr = bw_csr[my_beg];
            if (scc_id[vert_id] == 0 && fw_sa[nebr] != -1) {
              fw_sa[vert_id] = level + 1;
              vertex_frontier++;

              break;
            }
          }
        }
      }
    } else {
      std::vector<index_t> q(queue_size);
      index_t head = 0;
      index_t tail = 0;

      for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
        vertex_t vert_id = frontier_queue[fq_vert_id];
        if (scc_id[vert_id] == 0 && fw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }
      while (head != tail) {
        vertex_t temp_v = q[head++];

        if (head == queue_size)
          head = 0;
        index_t my_beg = fw_beg_pos[temp_v];
        index_t my_end = fw_beg_pos[temp_v + 1];

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
    }

    vertex_front[tid] = vertex_frontier;

#pragma omp barrier
    vertex_frontier = 0;

    for (index_t i = 0; i < thread_count; ++i) {
      vertex_frontier += vertex_front[i];
    }
    vertex_visited += vertex_frontier;

    if (vertex_frontier == 0)
      break;

#pragma omp barrier

    if (is_top_down) {
      const auto edge_frontier = static_cast<double>(vertex_frontier) * avg_degree;
      const auto edge_remainder = static_cast<double>(fq_size - vertex_visited) * avg_degree;
      if (edge_remainder / alpha < edge_frontier) {
        is_top_down = false;
      }
    } else if (!is_top_down_queue && (fq_size * 1.0 / beta) > vertex_frontier) {
      is_top_down_queue = true;
    }
#pragma omp barrier
    level++;
  }
}

inline void
bw_bfs_fq(
  index_t* scc_id,
  const index_t* fw_beg_pos,
  const index_t* bw_beg_pos,
  index_t vert_beg,
  index_t vert_end,
  const vertex_t* fw_csr,
  const vertex_t* bw_csr,
  const vertex_t* fw_sa,
  vertex_t* bw_sa,
  index_t* vertex_front,
  vertex_t root,
  index_t tid,
  index_t thread_count,
  double alpha,
  double beta,
  const vertex_t* frontier_queue,
  vertex_t fq_size,
  const double avg_degree,
  vertex_t vertex_visited)
{
  bw_sa[root] = 0;
  const vertex_t root_in_degree = bw_beg_pos[root + 1] - bw_beg_pos[root];
  bool is_top_down = true;
  bool is_top_down_queue = false;
  bool is_top_down_async = false;
  if (root_in_degree < alpha * beta * fq_size) {
    is_top_down_async = true;
  }
  index_t level = 0;
  scc_id[root] = 1;
  const index_t queue_size = fq_size / thread_count;

  while (true) {
    vertex_t vertex_frontier = 0;

    if (is_top_down) {
      if (is_top_down_async) {
        for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
          const vertex_t vert_id = frontier_queue[fq_vert_id];
          if (scc_id[vert_id] == 1 && (bw_sa[vert_id] == level || bw_sa[vert_id] == level + 1)) {
            index_t my_beg = bw_beg_pos[vert_id];
            const index_t my_end = bw_beg_pos[vert_id + 1];

            for (; my_beg < my_end; my_beg++) {
              const vertex_t nebr = bw_csr[my_beg];
              if (scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1) {
                bw_sa[nebr] = level + 1;

                vertex_frontier++;
                scc_id[nebr] = 1;
              }
            }
          }
        }
      } else {
        for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
          const vertex_t vert_id = frontier_queue[fq_vert_id];
          if (scc_id[vert_id] == 1 && bw_sa[vert_id] == level) {
            index_t my_beg = bw_beg_pos[vert_id];
            const index_t my_end = bw_beg_pos[vert_id + 1];

            for (; my_beg < my_end; my_beg++) {
              const vertex_t nebr = bw_csr[my_beg];
              if (scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1) {
                bw_sa[nebr] = level + 1;

                vertex_frontier++;

                scc_id[nebr] = 1;
              }
            }
          }
        }
      }
    } else if (!is_top_down_queue) {
      for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
        vertex_t vert_id = frontier_queue[fq_vert_id];

        if (scc_id[vert_id] == 0) {
          index_t my_beg = fw_beg_pos[vert_id];
          index_t my_end = fw_beg_pos[vert_id + 1];

          for (; my_beg < my_end; my_beg++) {
            vertex_t nebr = fw_csr[my_beg];

            if (scc_id[nebr] == 1) {
              bw_sa[vert_id] = level + 1;

              vertex_frontier++;
              scc_id[vert_id] = 1;
              break;
            }
          }
        }
      }
    } else {
      std::vector<index_t> q(queue_size);
      index_t head = 0;
      index_t tail = 0;

      for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; fq_vert_id++) {
        vertex_t vert_id = frontier_queue[fq_vert_id];
        if (scc_id[vert_id] == 1 && bw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }

      while (head != tail) {
        vertex_t temp_v = q[head++];

        if (head == queue_size)
          head = 0;
        index_t my_beg = bw_beg_pos[temp_v];
        index_t my_end = bw_beg_pos[temp_v + 1];

        for (; my_beg < my_end; ++my_beg) {
          index_t w = bw_csr[my_beg];

          if (scc_id[w] == 0 && bw_sa[w] == -1 && fw_sa[w] != -1) {
            q[tail++] = w;
            if (tail == queue_size)
              tail = 0;
            scc_id[w] = 1;
            bw_sa[w] = level + 1;
          }
        }
      }
    }

    vertex_front[tid] = vertex_frontier;

#pragma omp barrier
    vertex_frontier = 0;

    for (index_t i = 0; i < thread_count; ++i) {
      vertex_frontier += vertex_front[i];
    }
    vertex_visited += vertex_frontier;

    if (vertex_frontier == 0)
      break;

#pragma omp barrier

    if (is_top_down) {
      const auto edge_frontier = static_cast<double>(vertex_frontier) * avg_degree;
      const auto edge_remainder = static_cast<double>(fq_size - vertex_visited) * avg_degree;
      if (edge_remainder / alpha < edge_frontier) {
        is_top_down = false;
      }
    } else if (!is_top_down_queue && static_cast<double> (fq_size) / beta > vertex_frontier) {
      is_top_down_queue = true;
    }
#pragma omp barrier
    level++;
  }
  if (tid == 0)
    printf("bw_level, %lu\n", level);
}

inline void
process_wcc(
  index_t vert_beg,
  index_t vert_end,
  std::vector<vertex_t>& wcc_fq,
  const std::vector<vertex_t>& color,
  vertex_t& wcc_fq_size)
{
  std::set<vertex_t> s_fq;
  for (vertex_t i = vert_beg; i < vert_end; ++i) {
    if (s_fq.find(i) == s_fq.end())
      s_fq.insert(color[i]);
  }

  wcc_fq_size = s_fq.size();
  std::set<vertex_t>::iterator it;
  index_t i = 0;
  for (it = s_fq.begin(); it != s_fq.end(); ++it, ++i) {
    wcc_fq[i] = *it;
  }
}

inline void
mice_fw_bw(
  const std::vector<color_t>& wcc_color,
  std::vector<index_t>& scc_id,
  const std::vector<index_t>& sub_fw_beg,
  const std::vector<index_t>& sub_bw_beg,
  const std::vector<vertex_t>& sub_fw_csr,
  const std::vector<vertex_t>& sub_bw_csr,
  std::vector<vertex_t>& fw_sa,
  index_t tid,
  index_t thread_count,
  vertex_t sub_v_count,
  const std::vector<vertex_t>& wcc_fq,
  vertex_t wcc_fq_size)
{
  const index_t step = wcc_fq_size / thread_count;
  const index_t wcc_beg = tid * step;
  const index_t wcc_end = (tid == thread_count - 1 ? wcc_fq_size : wcc_beg + step);

  std::vector<index_t> q(sub_v_count);
  index_t head = 0;
  index_t tail = 0;

  for (vertex_t v = 0; v < sub_v_count; ++v) {
    if (scc_id[v] == -1) {
      const vertex_t cur_wcc = wcc_color[v];
      bool in_wcc = false;
      for (vertex_t i = wcc_beg; i < wcc_end; ++i) {
        if (wcc_fq[i] == cur_wcc) {
          in_wcc = true;
          break;
        }
      }
      if (in_wcc) {
        fw_sa[v] = v;
        q[tail++] = v;
        if (tail == sub_v_count)
          tail = 0;

        while (head != tail) {
          vertex_t temp_v = q[head++];

          if (head == sub_v_count)
            head = 0;
          index_t my_beg = sub_fw_beg[temp_v];
          index_t my_end = sub_fw_beg[temp_v + 1];

          for (; my_beg < my_end; ++my_beg) {
            index_t w = sub_fw_csr[my_beg];

            if (fw_sa[w] != v) {
              q[tail++] = w;
              if (tail == sub_v_count)
                tail = 0;
              fw_sa[w] = v;
            }
          }
        }

        scc_id[v] = v;
        q[tail++] = v;
        if (tail == sub_v_count)
          tail = 0;

        while (head != tail) {
          vertex_t temp_v = q[head++];

          if (head == sub_v_count)
            head = 0;
          index_t my_beg = sub_bw_beg[temp_v];
          index_t my_end = sub_bw_beg[temp_v + 1];

          for (; my_beg < my_end; ++my_beg) {
            index_t w = sub_bw_csr[my_beg];

            if (scc_id[w] == -1 && fw_sa[w] == v) {
              q[tail++] = w;
              if (tail == sub_v_count)
                tail = 0;
              scc_id[w] = v;
            }
          }
        }
      }
    }
  }
}
