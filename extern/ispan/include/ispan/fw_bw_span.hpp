#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <ispan/util.hpp>

#include <mpi.h>

#include <cstring>
#include <set>

inline void
fw_span(
  kaspan::vertex_t const* scc_id,
  kaspan::index_t const*  fw_head,
  kaspan::index_t const*  bw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::vertex_t const* bw_csr,
  kaspan::vertex_t        local_beg,
  kaspan::vertex_t        local_end,
  depth_t*                fw_sa,
  kaspan::index_t*        front_comm,
  kaspan::vertex_t        root,
  double                  alpha,
  kaspan::vertex_t        n,
  kaspan::index_t         m,
  kaspan::vertex_t        step,
  kaspan::vertex_t*       fq_comm,
  unsigned int*           sa_compress)
{
  depth_t level                = 0;
  fw_sa[root]                  = level;
  bool       is_top_down       = true;
  bool       is_top_down_queue = false;
  auto const queue_size        = std::max<kaspan::index_t>(1, n / 100);

  while (true) {
    kaspan::index_t front_count = 0;
    kaspan::index_t next_work   = 0;

    if (is_top_down) { // TOP DOWN STRATEGY

      for (auto u = local_beg; u < local_end; ++u) {
        if (scc_id[u] == kaspan::scc_id_undecided && fw_sa[u] == level) {

          auto const beg = fw_head[u];
          auto const end = fw_head[u + 1];
          for (auto it = beg; it < end; ++it) {
            auto const v = fw_csr[it];

            if (scc_id[v] == kaspan::scc_id_undecided && fw_sa[v] == depth_unset) {
              fw_sa[v] = kaspan::integral_cast<depth_t>(level + 1);
              sa_compress[v / 32] |= kaspan::integral_cast<kaspan::index_t>(1) << (v % 32);
              next_work += fw_head[v + 1] - fw_head[v];
              fq_comm[front_count] = v;
              ++front_count;
            }
          }
        }
      }

    } else if (!is_top_down_queue) {

      for (kaspan::vertex_t u = local_beg; u < local_end; u++) {
        if (scc_id[u] == kaspan::scc_id_undecided && fw_sa[u] == depth_unset) {

          auto const beg = bw_head[u];
          auto const end = bw_head[u + 1];
          next_work += end - beg;
          for (auto it = beg; it < end; ++it) {
            auto const v = bw_csr[it];

            if (scc_id[u] == kaspan::scc_id_undecided && fw_sa[v] != depth_unset) {
              fw_sa[u]             = kaspan::integral_cast<depth_t>(level + 1);
              fq_comm[front_count] = u;
              sa_compress[u / 32] |= 1 << (u % 32);
              front_count++;
              break;
            }
          }
        }
      }

    } else { // TOP DOWN QUEUE STRATEGY

      auto* q = new kaspan::index_t[queue_size]{};
      SCOPE_GUARD(delete[] q);

      kaspan::index_t head = 0;
      kaspan::index_t tail = 0;
      for (kaspan::vertex_t vert_id = local_beg; vert_id < local_end; vert_id++) {
        if (scc_id[vert_id] == kaspan::scc_id_undecided && fw_sa[vert_id] == level) {
          q[tail++] = vert_id;
        }
      }
      while (head != tail) {
        auto const temp_v = q[head++];
        if (head == queue_size) head = 0;

        auto const beg = fw_head[temp_v];
        auto const end = fw_head[temp_v + 1];
        for (auto it = beg; it < end; ++it) {
          auto const w = fw_csr[it];

          if (scc_id[w] == kaspan::scc_id_undecided && fw_sa[w] == depth_unset) {
            q[tail++] = w;
            if (tail == queue_size) tail = 0;
            fw_sa[w] = kaspan::integral_cast<depth_t>(level + 1);
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

    front_comm[kaspan::mpi_basic::world_rank] = front_count;
    if (is_top_down) {
      // clang-format off
      MPI_Allreduce_c(
        MPI_IN_PLACE, &next_work, 1, kaspan::mpi_index_t,
        MPI_SUM, MPI_COMM_WORLD);
      // clang-format on
    }

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, &front_count, 1, kaspan::mpi_basic::type<decltype(front_count)>,
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
        MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        fw_sa, step, mpi_depth_t,
        MPI_COMM_WORLD);
      // clang-format on
    }
    // if (!is_top_down || is_top_down_queue) { // -(???)
    if (front_count > 0) { // +(???)
      if (front_count > 10000) {

        kaspan::index_t s = n / 32;
        if (n % 32 != 0) s += 1;

        // clang-format off
        MPI_Allreduce(
          MPI_IN_PLACE, sa_compress, s, kaspan::mpi_basic::type<decltype(*sa_compress)>,
          MPI_BOR, MPI_COMM_WORLD);
        // clang-format on

        kaspan::vertex_t num_sa = 0;
        for (kaspan::index_t i = 0; i < n; ++i) {
          if (scc_id[i] == kaspan::scc_id_undecided && fw_sa[i] == depth_unset && (sa_compress[i / 32] & 1 << (i % 32)) != 0) {
            fw_sa[i] = kaspan::integral_cast<depth_t>(level + 1);
            num_sa += 1;
          }
        }
      } else {

        auto const rank_count = front_comm[kaspan::mpi_basic::world_rank];

        // clang-format off
        // option 1:
        // MPI_Allgather(
        //   &rank_count, 1, kaspan::mpi_basic::type<decltype(rank_count)>,
        //   front_comm, 1, kaspan::mpi_basic::type<decltype(*front_comm)>,
        //   MPI_COMM_WORLD);
        // option 2:
        memset(front_comm, 0, kaspan::mpi_basic::world_size * sizeof(*front_comm));
        front_comm[kaspan::mpi_basic::world_rank] = rank_count;
        MPI_Allreduce(
          MPI_IN_PLACE, front_comm, kaspan::mpi_basic::world_size, kaspan::mpi_basic::type<decltype(*front_comm)>,
          MPI_MAX, MPI_COMM_WORLD);
        // clang-format on

        MPI_Request request;
        for (int i = 0; i < kaspan::mpi_basic::world_size; ++i) {
          if (i != kaspan::mpi_basic::world_rank) {

            // clang-format off
            MPI_Isend(
              fq_comm, front_comm[kaspan::mpi_basic::world_rank], kaspan::mpi_basic::type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, &request);
            // clang-format on

            MPI_Request_free(&request);
          }
        }
        auto fq_begin = front_comm[kaspan::mpi_basic::world_rank];
        for (int i = 0; i < kaspan::mpi_basic::world_size; ++i) {
          if (i != kaspan::mpi_basic::world_rank) {

            // clang-format off
            MPI_Recv(
              fq_comm + fq_begin, front_comm[i], kaspan::mpi_basic::type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // clang-format on

            fq_begin += front_comm[i];
          }
        }
        for (int i = front_comm[kaspan::mpi_basic::world_rank]; i < front_count; ++i) {
          auto const v = fq_comm[i];
          if (fw_sa[v] == depth_unset) {
            fw_sa[v] = kaspan::integral_cast<depth_t>(level + 1);
          }
        }
      }
    }
    level++;
  }
}

inline void
bw_span(
  kaspan::vertex_t*       scc_id,
  kaspan::index_t const*  fw_head,
  kaspan::index_t const*  bw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::vertex_t const* bw_csr,

  kaspan::vertex_t local_beg,
  kaspan::vertex_t local_end,

  depth_t const*   fw_sa,
  depth_t*         bw_sa,
  kaspan::index_t* front_comm,
  int*             work_comm,

  kaspan::vertex_t  root,
  double            alpha,
  kaspan::vertex_t  n,
  kaspan::vertex_t  m,
  kaspan::vertex_t  step,
  kaspan::vertex_t* fq_comm,
  unsigned int*     sa_compress)
{
  size_t decided_count = 0;

  bw_sa[root]               = 0;
  bool    is_top_down       = true;
  bool    is_top_down_queue = false;
  depth_t level             = 0;
  scc_id[root]              = scc_id_largest;
  ++decided_count;
  auto const queue_size = std::max<kaspan::index_t>(1, n / 100);

  while (true) {
    kaspan::index_t front_count = 0;
    int             next_work   = 0;
    if (is_top_down) {
      for (auto u = local_beg; u < local_end; u++) {
        if (bw_sa[u] == level) {

          auto const beg = bw_head[u];
          auto const end = bw_head[u + 1];
          for (auto it = beg; it < end; ++it) {
            auto const v = bw_csr[it];

            if (bw_sa[v] == depth_unset && fw_sa[v] != depth_unset) {
              bw_sa[v] = kaspan::integral_cast<depth_t>(level + 1);
              next_work += bw_head[v + 1] - bw_head[v];
              fq_comm[front_count] = v;
              front_count++;
              scc_id[v] = scc_id_largest;
              ++decided_count;
              sa_compress[v / 32] |= 1 << (v % 32);
            }
          }
        }
      }
      work_comm[kaspan::mpi_basic::world_rank] = next_work;
    } else if (!is_top_down_queue) {
      for (auto u = local_beg; u < local_end; u++) {
        if (bw_sa[u] == depth_unset && fw_sa[u] != depth_unset) {

          auto const beg = fw_head[u];
          auto const end = fw_head[u + 1];
          next_work += end - beg;
          for (auto it = beg; it < end; it++) {
            auto const v = fw_csr[it];

            if (bw_sa[v] != depth_unset) {
              bw_sa[u]             = kaspan::integral_cast<depth_t>(level + 1);
              fq_comm[front_count] = u;
              front_count++;
              sa_compress[u / 32] |= 1 << (u % 32);
              scc_id[u] = scc_id_largest;
              ++decided_count;
              break;
            }
          }
        }
      }
      work_comm[kaspan::mpi_basic::world_rank] = next_work;
    } else {

      auto* q = new kaspan::index_t[queue_size]{};
      SCOPE_GUARD(delete[] q);

      kaspan::index_t head = 0;
      kaspan::index_t tail = 0;
      for (kaspan::vertex_t vert_id = local_beg; vert_id < local_end; vert_id++) {
        if (bw_sa[vert_id] == level) q[tail++] = vert_id;
      }
      while (head != tail) {
        auto const u = q[head++];
        if (head == queue_size) head = 0;

        auto const beg = bw_head[u];
        auto const end = bw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = bw_csr[it];

          if (bw_sa[v] == depth_unset && fw_sa[v] != depth_unset) {
            q[tail++] = v;
            if (tail == queue_size) tail = 0;
            scc_id[v] = scc_id_largest;
            ++decided_count;
            bw_sa[v] = kaspan::integral_cast<depth_t>(level + 1);
          }
        }
      }

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id, n, kaspan::mpi_vertex_t,
        MPI_MIN, MPI_COMM_WORLD);
      // clang-format on

      break;
    }
    front_comm[kaspan::mpi_basic::world_rank] = front_count;

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, &front_count, 1, kaspan::mpi_basic::type<decltype(front_count)>,
      MPI_SUM, MPI_COMM_WORLD);
    // clang-format on

    if (is_top_down) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, &next_work, 1, kaspan::mpi_basic::type<decltype(next_work)>,
        MPI_SUM, MPI_COMM_WORLD);
      // clang-format on
    }
    if (front_count == 0) {

      // clang-format off
      MPI_Allreduce(
        MPI_IN_PLACE, scc_id, n, kaspan::mpi_vertex_t,
        MPI_MIN, MPI_COMM_WORLD);
      // clang-format on

      break;
    }
    if (is_top_down && next_work * alpha > m) {
      is_top_down = false;
    } else if (level > 50) {
      is_top_down_queue = true;

      // clang-format off
      ASSERT_EQ(local_beg, kaspan::mpi_basic::world_rank * step);
      MPI_Allgather(
        MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        bw_sa, step, mpi_depth_t,
        MPI_COMM_WORLD);
      // clang-format on
    }

    if (front_count > 0) {
      if (front_count > 10000) {

        kaspan::index_t s = n / 32;
        if (n % 32 != 0) s += 1;

        // clang-format off
        MPI_Allreduce(
          MPI_IN_PLACE, sa_compress, s, kaspan::mpi_basic::type<decltype(*sa_compress)>,
          MPI_BOR, MPI_COMM_WORLD);
        // clang-format on

        kaspan::vertex_t num_sa = 0;
        for (kaspan::vertex_t i = 0; i < n; ++i) {
          if (fw_sa[i] != depth_unset && bw_sa[i] == depth_unset && (sa_compress[i / 32] & 1 << (i % 32)) != 0) {
            bw_sa[i] = kaspan::integral_cast<depth_t>(level + 1);
            num_sa += 1;
          }
        }
      } else {

        auto const rank_count = front_comm[kaspan::mpi_basic::world_rank];

        // clang-format off
        // option 1:
        // MPI_Allgather(
        //   &rank_count, 1, kaspan::mpi_basic::type<decltype(rank_count)>,
        //   front_comm, 1, kaspan::mpi_basic::type<decltype(*front_comm)>,
        //   MPI_COMM_WORLD);
        // option 2:
        memset(front_comm, 0, kaspan::mpi_basic::world_size * sizeof(*front_comm));
        front_comm[kaspan::mpi_basic::world_rank] = rank_count;
        MPI_Allreduce(
          MPI_IN_PLACE, front_comm, kaspan::mpi_basic::world_size, kaspan::mpi_basic::type<decltype(*front_comm)>,
          MPI_MAX, MPI_COMM_WORLD);
        // clang-format on

        MPI_Request request;
        for (int i = 0; i < kaspan::mpi_basic::world_size; ++i) {
          if (i != kaspan::mpi_basic::world_rank) {

            // clang-format off
            MPI_Isend(
              fq_comm, front_comm[kaspan::mpi_basic::world_rank], kaspan::mpi_basic::type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, &request);
            // clang-format on

            MPI_Request_free(&request);
          }
        }
        auto fq_begin = front_comm[kaspan::mpi_basic::world_rank];
        for (int i = 0; i < kaspan::mpi_basic::world_size; ++i) {
          if (i != kaspan::mpi_basic::world_rank) {

            // clang-format off
            MPI_Recv(
              fq_comm + fq_begin, front_comm[i], kaspan::mpi_basic::type<decltype(*fq_comm)>,
              i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // clang-format on

            fq_begin += front_comm[i];
          }
        }
        for (int i = 0; i < front_count; ++i) {
          auto const v_new = fq_comm[i];
          if (bw_sa[v_new] == depth_unset) {
            bw_sa[v_new] = kaspan::integral_cast<depth_t>(level + 1);
          }
        }
      }
    }
    level++;
  }

  KASPAN_STATISTIC_ADD("decided_count", decided_count);
}
