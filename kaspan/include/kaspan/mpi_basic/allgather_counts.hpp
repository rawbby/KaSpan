#pragma once

#include <kaspan/mpi_basic/allgather.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief Allgather a single count from each rank.
 *
 * @param send_count Local count.
 * @param counts Out array of length world_size receiving one MPI_Count per rank.
 */
inline void
allgather_counts(MPI_Count send_count, MPI_Count* counts)
{
  allgather(&send_count, 1, MPI_COUNT, counts, 1, MPI_COUNT);
}

} // namespace kaspan::mpi_basic
