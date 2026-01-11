#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/world.hpp>

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

  KASPAN_VALGRIND_CHECK_MEM_IS_DEFINED(send_counts, world_size * sizeof(MPI_Count));
  KASPAN_VALGRIND_CHECK_MEM_IS_ADDRESSABLE(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(recv_counts, world_size * sizeof(MPI_Count));

  [[maybe_unused]] auto const rc = MPI_Alltoall_c(send_counts, 1, MPI_COUNT, recv_counts, 1, MPI_COUNT, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_VALGRIND_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
}

} // namespace kaspan::mpi_basic
