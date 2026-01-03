#pragma once

#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief Calculate displacements from counts.
 *
 * @param counts Per-rank element counts.
 * @param displs Out array of displacements.
 * @return Total number of elements.
 */
inline auto
displs(MPI_Count const* counts, MPI_Aint* displs) -> MPI_Count
{
  DEBUG_ASSERT_NE(counts, nullptr);
  DEBUG_ASSERT_NE(displs, nullptr);
  MPI_Count count = 0;
  for (i32 i = 0; i < world_size; ++i) {
    DEBUG_ASSERT_GE(counts[i], 0);
    displs[i] = static_cast<MPI_Aint>(count);
    count += counts[i];
  }
  return count;
}

} // namespace kaspan::mpi_basic
