#pragma once

#include <comm/MpiBasic.hpp>
#include <ispan/util.hpp>
#include <test/Assert.hpp>
#include <util/ScopeGuard.hpp>

#include <cstring>
#include <mpi.h>
#include <set>

inline void
fw_span(
  vertex_t const*  scc_id,
  index_t const*  fw_head,
  index_t const*  bw_head,
  vertex_t const* fw_csr,
  vertex_t const* bw_csr,
  vertex_t        local_beg,
  vertex_t        local_end,
  depth_t*    fw_sa,
  index_t*        front_comm,
  vertex_t        root,
  index_t         world_rank,
  index_t         world_size,
  double          alpha,
  vertex_t        n,
  index_t         m,
  vertex_t        step,
  vertex_t*       fq_comm,
  unsigned int*   sa_compress)
{
  depth_t level            = 0;
  fw_sa[root]                  = 0;
  bool       is_top_down       = true;
  bool       is_top_down_queue = false;
  auto const queue_size        = std::max<index_t>(1, n / 100);

  while (true) {
    index_t front_count = 0;
    int     next_work   = 0;

    if (is_top_down) { // TOP DOWN STRATEGY

      for (auto u = local_beg; u < local_end; ++u) {
        if (scc_id[u] == scc_id_undecided and fw_sa[u] == level) {

          auto const beg = fw_head[u];
          auto const end = fw_head[u + 1];
          for (auto it = beg; it < end; ++it) {
            auto const v = fw_csr[it];

            if (scc_id[v] == scc_id_undecided && fw_sa[v] == depth_unset) {
              fw_sa[v] = static_cast<depth_t>(level + 1);
              sa_compress[v / 32] |= 1 << (static_cast<index_t>(v) % 32);
              next_work += fw_head[v + 1] - fw_head[v];
              fq_comm[front_count] = v;
              front_count++;
            }
          }
        }
      }

    } else if (!is_top_down_queue) {

      for (vertex_t u = local_beg; u < local_end; u++) {
        if (scc_id[u] == scc_id_undecided && fw_sa[u] == depth_unset) {

          auto const beg = bw_head[u];
          auto const end = bw_head[u + 1];
          next_work += end - beg;
          for (auto it = beg; it < end; ++it) {
            auto const v = bw_csr[it];

            if (scc_id[u] == scc_id_undecided && fw_sa[v] != depth_unset) {
              fw_sa[u]             = static_cast<depth_t>(level + 1);
              fq_comm[front_count] = u;
              sa_compress[u / 32] |= 1 << (u % 32);
              front_count++;
              break;
            }
          }
        }
      }

    } else { // TOP DOWN QUEUE STRATEGY

      auto* q = new index_t[queue_size]{};
      SCOPE_GUARD(delete[] q);

      index_t head = 0;
      index_t tail = 0;
      for (vertex_t vert_id = local_beg; vert_id < local_end; vert_id++) {
        if (scc_id[vert_id] == scc_id_undecided && fw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }
      while (head != tail) {
        auto const temp_v = q[head++];
        if (head == queue_size)
          head = 0;

        auto const beg = fw_head[temp_v];
        auto const end = fw_head[temp_v + 1];
        for (auto it = beg; it < end; ++it) {
          auto const w = fw_csr[it];

          if (scc_id[w] == scc_id_undecided && fw_sa[w] == depth_unset) {
            q[tail++] = w;
            if (tail == queue_size)
              tail = 0;
            fw_sa[w] = static_cast<depth_t>(level + 1);
          }
        }
      }

      // clang-format off
      MPI_Allreduce_c(
        MPI_IN_PLACE, fw_sa, n, mpi_depth_t,
        MPI_MAX, MPI_COMM_WORLD);
      // clang-format on

      break;
    }

    front_comm[world_rank] = front_count;
    if (is_top_down) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, &next_work, 1, mpi_basic_type<decltype(next_work)>,
        MPI_SUM, MPI_COMM_WORLD);
      // clang-format on
    }

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, &front_count, 1, mpi_basic_type<decltype(front_count)>,
      MPI_SUM, MPI_COMM_WORLD);
    // clang-format on

    if (front_count == 0) {

      // clang-format off
      MPI_Allreduce_c(
        MPI_IN_PLACE, fw_sa, n, mpi_depth_t,
        MPI_MAX, MPI_COMM_WORLD);
      // clang-format on

      break;
    }

    if (is_top_down && next_work * alpha > m) {
      is_top_down = false;
    } else if (level > 50) {
      is_top_down_queue = true;

      // clang-format off
      MPI_Allgather(
        /* send: */ fw_sa + local_beg, step, mpi_depth_t,
        /* recv: */ fw_sa, step, mpi_depth_t,
        /* comm: */ MPI_COMM_WORLD);
      // clang-format on
    }
    // if (!is_top_down || is_top_down_queue) { // -(???)
    if (front_count > 0) { // +(???)
      if (front_count > 10000) {

        index_t s = n / 32;
        if (n % 32 != 0)
          s += 1;

        // clang-format off
        MPI_Allreduce(
          MPI_IN_PLACE, sa_compress, s, mpi_basic_type<decltype(*sa_compress)>,
          MPI_BOR, MPI_COMM_WORLD);
        // clang-format on

        vertex_t num_sa = 0;
        for (index_t i = 0; i < n; ++i) {
          if (scc_id[i] == scc_id_undecided && fw_sa[i] == depth_unset && (sa_compress[i / 32] & 1 << (i % 32)) != 0) {
            fw_sa[i] = static_cast<depth_t>(level + 1);
            num_sa += 1;
          }
        }
      } else {

        auto const rank_count = front_comm[world_rank];

        // clang-format off
        // option 1:
        // MPI_Allgather(
        //   &rank_count, 1, mpi_basic_type<decltype(rank_count)>,
        //   front_comm, 1, mpi_basic_type<decltype(*front_comm)>,
        //   MPI_COMM_WORLD);
        // option 2:
        memset(front_comm, 0, world_size * sizeof(*front_comm));
        front_comm[world_rank] = rank_count;
        MPI_Allreduce(
          MPI_IN_PLACE, front_comm, world_size, mpi_basic_type<decltype(*front_comm)>,
          MPI_MAX, MPI_COMM_WORLD);
        // clang-format on

        MPI_Request request;
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {

            // clang-format off
            MPI_Isend(
              fq_comm, front_comm[world_rank], mpi_basic_type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, &request);
            // clang-format on

            MPI_Request_free(&request);
          }
        }
        auto fq_begin = front_comm[world_rank];
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {

            // clang-format off
            MPI_Recv(
              fq_comm + fq_begin, front_comm[i], mpi_basic_type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // clang-format on

            fq_begin += front_comm[i];
          }
        }
        for (int i = front_comm[world_rank]; i < front_count; ++i) {
          auto const v = fq_comm[i];
          if (fw_sa[v] == depth_unset) {
            fw_sa[v] = static_cast<depth_t>(level + 1);
          }
        }
      }
    }
    level++;
  }
}

inline void
bw_span(
  vertex_t*       scc_id,
  index_t const*  fw_head,
  index_t const*  bw_head,
  vertex_t const* fw_csr,
  vertex_t const* bw_csr,

  vertex_t local_beg,
  vertex_t local_end,

  depth_t const* fw_sa,
  depth_t*       bw_sa,
  index_t*           front_comm,
  int*               work_comm,

  vertex_t      root,
  index_t       world_rank,
  index_t       world_size,
  double        alpha,
  vertex_t      n,
  vertex_t      m,
  vertex_t      step,
  vertex_t*     fq_comm,
  unsigned int* sa_compress)
{
  bw_sa[root]                   = 0;
  bool        is_top_down       = true;
  bool        is_top_down_queue = false;
  depth_t level             = 0;
  scc_id[root]                  = scc_id_largest;
  auto const queue_size         = std::max<index_t>(1, n / 100);

  while (true) {
    index_t front_count = 0;
    int     next_work   = 0;
    if (is_top_down) {
      for (auto u = local_beg; u < local_end; u++) {
        if (bw_sa[u] == level) {

          auto const beg = bw_head[u];
          auto const end = bw_head[u + 1];
          for (auto it = beg; it < end; ++it) {
            auto const v = bw_csr[it];

            if (bw_sa[v] == depth_unset && fw_sa[v] != depth_unset) {
              bw_sa[v] = static_cast<depth_t>(level + 1);
              next_work += bw_head[v + 1] - bw_head[v];
              fq_comm[front_count] = v;
              front_count++;
              scc_id[v] = scc_id_largest;
              sa_compress[v / 32] |= 1 << (v % 32);
            }
          }
        }
      }
      work_comm[world_rank] = next_work;
    } else if (!is_top_down_queue) {
      for (auto u = local_beg; u < local_end; u++) {
        if (bw_sa[u] == depth_unset && fw_sa[u] != depth_unset) {

          auto const beg = fw_head[u];
          auto const end = fw_head[u + 1];
          next_work += end - beg;
          for (auto it = beg; it < end; it++) {
            auto const v = fw_csr[it];

            if (bw_sa[v] != depth_unset) {
              bw_sa[u]             = static_cast<depth_t>(level + 1);
              fq_comm[front_count] = u;
              front_count++;
              sa_compress[u / 32] |= 1 << (u % 32);
              scc_id[u] = scc_id_largest;
              break;
            }
          }
        }
      }
      work_comm[world_rank] = next_work;
    } else {

      auto* q = new index_t[queue_size]{};
      SCOPE_GUARD(delete[] q);

      index_t head = 0;
      index_t tail = 0;
      for (vertex_t vert_id = local_beg; vert_id < local_end; vert_id++) {
        if (bw_sa[vert_id] == level)
          q[tail++] = vert_id;
      }
      while (head != tail) {
        auto const u = q[head++];
        if (head == queue_size)
          head = 0;

        auto const beg = bw_head[u];
        auto const end = bw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = bw_csr[it];

          if (bw_sa[v] == depth_unset && fw_sa[v] != depth_unset) {
            q[tail++] = v;
            if (tail == queue_size)
              tail = 0;
            scc_id[v] = scc_id_largest;
            bw_sa[v]  = static_cast<depth_t>(level + 1);
          }
        }
      }

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id, n, mpi_vertex_t,
        MPI_MIN, MPI_COMM_WORLD);
      // clang-format on

      break;
    }
    front_comm[world_rank] = front_count;

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, &front_count, 1, mpi_basic_type<decltype(front_count)>,
      MPI_SUM, MPI_COMM_WORLD);
    // clang-format on

    if (is_top_down) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, &next_work, 1, mpi_basic_type<decltype(next_work)>,
        MPI_SUM, MPI_COMM_WORLD);
      // clang-format on
    }
    if (front_count == 0) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id, n, mpi_vertex_t,
        MPI_MIN, MPI_COMM_WORLD);
      // clang-format on

      break;
    }
    if (is_top_down and next_work * alpha > m) {
      is_top_down = false;
    } else if (level > 50) {
      is_top_down_queue = true;

      // clang-format off
      MPI_Allgather(
        /* send: */ bw_sa + local_beg, step, mpi_depth_t,
        /* recv: */ bw_sa, step, mpi_depth_t,
        /* comm: */ MPI_COMM_WORLD);
      // clang-format on
    }

    if (front_count > 0) {
      if (front_count > 10000) {

        index_t s = n / 32;
        if (n % 32 != 0)
          s += 1;

        // clang-format off
        MPI_Allreduce(
          MPI_IN_PLACE, sa_compress, s, mpi_basic_type<decltype(*sa_compress)>,
          MPI_BOR, MPI_COMM_WORLD);
        // clang-format on

        vertex_t num_sa = 0;
        for (vertex_t i = 0; i < n; ++i) {
          if (fw_sa[i] != depth_unset and
              bw_sa[i] == depth_unset and
              (sa_compress[i / 32] & 1 << (i % 32)) != 0) {
            bw_sa[i] = static_cast<depth_t>(level + 1);
            // scc_id[i] = scc_id_largest; // +(???) this is added manually
            num_sa += 1;
          }
        }
      } else {

        auto const rank_count = front_comm[world_rank];

        // clang-format off
        // option 1:
        // MPI_Allgather(
        //   &rank_count, 1, mpi_basic_type<decltype(rank_count)>,
        //   front_comm, 1, mpi_basic_type<decltype(*front_comm)>,
        //   MPI_COMM_WORLD);
        // option 2:
        memset(front_comm, 0, world_size * sizeof(*front_comm));
        front_comm[world_rank] = rank_count;
        MPI_Allreduce(
          MPI_IN_PLACE, front_comm, world_size, mpi_basic_type<decltype(*front_comm)>,
          MPI_MAX, MPI_COMM_WORLD);
        // clang-format on

        MPI_Request request;
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {

            // clang-format off
            MPI_Isend(
              fq_comm, front_comm[world_rank], mpi_basic_type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, &request);
            // clang-format on

            MPI_Request_free(&request);
          }
        }
        auto fq_begin = front_comm[world_rank];
        for (int i = 0; i < world_size; ++i) {
          if (i != world_rank) {

            // clang-format off
            MPI_Recv(
              fq_comm + fq_begin, front_comm[i], mpi_basic_type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // clang-format on

            fq_begin += front_comm[i];
          }
        }
        for (int i = 0; i < front_count; ++i) {
          auto const v_new = fq_comm[i];
          if (bw_sa[v_new] == depth_unset) {
            bw_sa[v_new] = static_cast<depth_t>(level + 1);
          }
        }
      }
    }
    level++;
  }
}
