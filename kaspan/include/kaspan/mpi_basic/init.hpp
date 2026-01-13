#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/util/scope_guard.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <mpi.h>

namespace kaspan {

/**
 * @brief Initialize MPI (MPI_Init) and arrange automatic MPI_Finalize via SCOPE_GUARD.
 * Populates world_rank, world_size, and world_root.
 *
 * Call once per process before using the wrappers. Not thread-safe.
 */
#define MPI_INIT()                                                                                                                                                                 \
  {                                                                                                                                                                                \
    [[maybe_unused]] auto const rc0 = MPI_Init(nullptr, nullptr);                                                                                                                  \
    DEBUG_ASSERT_EQ(rc0, MPI_SUCCESS);                                                                                                                                             \
  }                                                                                                                                                                                \
  SCOPE_GUARD(MPI_Finalize());                                                                                                                                                     \
  {                                                                                                                                                                                \
    [[maybe_unused]] auto const rc1 = MPI_Comm_rank(MPI_COMM_WORLD, &kaspan::mpi_basic::world_rank);                                                                               \
    DEBUG_ASSERT_EQ(rc1, MPI_SUCCESS);                                                                                                                                             \
    [[maybe_unused]] auto const rc2 = MPI_Comm_size(MPI_COMM_WORLD, &kaspan::mpi_basic::world_size);                                                                               \
    DEBUG_ASSERT_EQ(rc2, MPI_SUCCESS);                                                                                                                                             \
    kaspan::mpi_basic::world_root = kaspan::mpi_basic::world_rank == 0;                                                                                                            \
  }                                                                                                                                                                                \
  KASPAN_MEMCHECK_PRINTF("[INIT] rank %d initialized\n", kaspan::mpi_basic::world_rank); /* this print should help to map the rank to the pid in valgrind logs */                  \
  ((void)0)

} // namespace kaspan
