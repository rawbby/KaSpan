#pragma once

#include <kaspan/memory/borrow.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/util/return_pack.hpp>
#include <mpi.h>

namespace kaspan {

namespace mpi_basic {

/**
 * @brief Borrow counts and displacements arrays from memory.
 *
 * @param memory Pointer to memory to borrow from.
 * @return PACK(counts, displs)
 */
inline auto
counts_and_displs(void** memory)
{
  auto* counts = borrow_array<MPI_Count>(memory, world_size);
  auto* displs = borrow_array<MPI_Aint>(memory, world_size);
  return PACK(counts, displs);
}

/**
 * @brief Allocate temporary counts and displacements arrays for MPI-4 "_c" collectives.
 *
 * @return PACK(buffer, counts, displs)
 */
inline auto
counts_and_displs()
{
  auto  buffer = make_buffer<MPI_Count, MPI_Aint>(world_size, world_size);
  auto* memory = buffer.data();
  auto* counts = borrow_array<MPI_Count>(&memory, world_size);
  auto* displs = borrow_array<MPI_Aint>(&memory, world_size);
  return PACK(buffer, counts, displs);
}

} // namespace mpi_basic

} // namespace kaspan
