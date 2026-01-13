#pragma once

#include <kaspan/debug/statistic.hpp>

#include <ispan/util.hpp>

inline void
trim_1_first(kaspan::vertex_t*      scc_id,
             kaspan::index_t const* fw_head,
             kaspan::index_t const* bw_head,
             kaspan::vertex_t       local_beg,
             kaspan::vertex_t       local_end,
             kaspan::vertex_t       step)
{
  KASPAN_STATISTIC_SCOPE("trim_1_first");

  size_t decided_count = 0;
  for (auto u = local_beg; u < local_end; ++u) {
    if (fw_head[u + 1] - fw_head[u] == 0 or bw_head[u + 1] - bw_head[u] == 0) {
      scc_id[u] = kaspan::scc_id_singular;
      ++decided_count;
    }
  }
  MPI_Allgather(MPI_IN_PLACE, 0, kaspan::mpi_vertex_t, scc_id, step, kaspan::mpi_vertex_t, MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("decided_count", decided_count);
}
