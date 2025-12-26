#pragma once

#include <debug/statistic.hpp>
#include <ispan/util.hpp>

inline void
trim_1_first(vertex_t* scc_id, index_t const* fw_head, index_t const* bw_head, vertex_t local_beg, vertex_t local_end, vertex_t step)
{
  KASPAN_STATISTIC_SCOPE("trim_1_first");

  size_t decided_count = 0;
  for (auto u = local_beg; u < local_end; ++u) {
    if (fw_head[u + 1] - fw_head[u] == 0 or bw_head[u + 1] - bw_head[u] == 0) {
      scc_id[u] = scc_id_singular;
      ++decided_count;
    }
  }
  MPI_Allgather(MPI_IN_PLACE, 0, mpi_vertex_t, scc_id, step, mpi_vertex_t, MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("decided_count", decided_count);
}
