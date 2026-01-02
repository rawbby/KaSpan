#pragma once

#include <mpi.h>
#include <util/scope_guard.hpp>
#include <mpi_basic/world.hpp>

/**
 * @brief Initialize MPI (MPI_Init) and arrange automatic MPI_Finalize via SCOPE_GUARD.
 * Populates world_rank, world_size, and world_root.
 *
 * Call once per process before using the wrappers. Not thread-safe.
 */
#define MPI_INIT()                                            \
  MPI_Init(nullptr, nullptr);                                 \
  SCOPE_GUARD(MPI_Finalize());                                \
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_basic::world_rank);      \
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_basic::world_size);      \
  mpi_basic::world_root = mpi_basic::world_rank == 0;
