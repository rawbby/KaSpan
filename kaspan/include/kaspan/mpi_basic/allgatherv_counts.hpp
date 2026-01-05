#pragma once

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief Allgather a single count from each rank.
 *
 * @param send_count Local count.
 * @param counts Out array of length world_size receiving one MPI_Count per rank.
 */
inline void
allgatherv_counts(MPI_Count send_count, MPI_Count* counts)
{
  DEBUG_ASSERT_NE(counts, nullptr);
  MPI_Allgather_c(&send_count, 1, MPI_COUNT, counts, 1, MPI_COUNT, MPI_COMM_WORLD);
}

} // namespace kaspan::mpi_basic
