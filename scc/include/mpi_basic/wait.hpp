#pragma once

#include <mpi.h>

namespace mpi_basic {

/**
 * @brief Wait for an MPI request to complete.
 */
inline void
wait(MPI_Request request)
{
  DEBUG_ASSERT_NE(request, MPI_REQUEST_NULL);
  MPI_Wait(&request, MPI_STATUS_IGNORE);
}

} // namespace mpi_basic
