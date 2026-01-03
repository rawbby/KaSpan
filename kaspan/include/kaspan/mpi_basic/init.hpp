#pragma once

#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/util/scope_guard.hpp>
#include <mpi.h>

namespace kaspan {

/**
 * @brief Initialize MPI (MPI_Init) and arrange automatic MPI_Finalize via SCOPE_GUARD.
 * Populates world_rank, world_size, and world_root.
 *
 * Call once per process before using the wrappers. Not thread-safe.
 */
#define MPI_INIT()                                                                                                                                                                 \
  MPI_Init(nullptr, nullptr);                                                                                                                                                      \
  SCOPE_GUARD(MPI_Finalize());                                                                                                                                                     \
  MPI_Comm_rank(MPI_COMM_WORLD, &kaspan::mpi_basic::world_rank);                                                                                                                   \
  MPI_Comm_size(MPI_COMM_WORLD, &kaspan::mpi_basic::world_size);                                                                                                                   \
  kaspan::mpi_basic::world_root = kaspan::mpi_basic::world_rank == 0;

} // namespace kaspan
