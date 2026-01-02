#pragma once

#include <util/arithmetic.hpp>
#include <mpi.h>

namespace mpi_basic {

/**
 * @brief The default communicator used by mpi_basic.
 */
inline MPI_Comm comm_world = MPI_COMM_WORLD;

/**
 * @brief Whether this rank is the root (rank 0).
 */
inline bool world_root = true;

/**
 * @brief The rank of the process in MPI_COMM_WORLD.
 */
inline i32  world_rank = 0;

/**
 * @brief The size of MPI_COMM_WORLD.
 */
inline i32  world_size = 1;

} // namespace mpi_basic
