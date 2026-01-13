#pragma once

#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/util/pp.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief MPI barrier on comm_world.
 */
inline void
barrier()
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Barrier(comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

} // namespace kaspan::mpi_basic
