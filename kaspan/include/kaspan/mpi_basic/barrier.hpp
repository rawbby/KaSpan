#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/mpi_basic/world.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief MPI barrier on comm_world.
 */
inline void
barrier()
{
  [[maybe_unused]] auto const rc = MPI_Barrier(comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
}

} // namespace kaspan::mpi_basic
