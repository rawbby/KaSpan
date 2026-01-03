#pragma once

#include <mpi.h>

namespace kaspan {

namespace mpi_basic {

/**
 * @brief MPI barrier on MPI_COMM_WORLD.
 */
inline void
barrier()
{
  MPI_Barrier(MPI_COMM_WORLD);
}

} // namespace mpi_basic

} // namespace kaspan
