#pragma once

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief All-to-all exchange of per-destination element counts (MPI-4 "_c" variant).
 *
 * @param send_counts Array of length world_size.
 * @param recv_counts Out array of length world_size.
 */
inline void
alltoallv_counts(MPI_Count const* send_counts, MPI_Count* recv_counts)
{
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  MPI_Alltoall_c(send_counts, 1, MPI_COUNT, recv_counts, 1, MPI_COUNT, MPI_COMM_WORLD);
}

} // namespace kaspan::mpi_basic
