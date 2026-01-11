#pragma once

#include <kaspan/mpi_basic/extent_of.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <kaspan/mpi_basic/world.hpp>

#include <mpi.h>
#include <numeric> // std::accumulate

namespace kaspan::mpi_basic {

/**
 * @brief Untyped wrapper for MPI_Alltoallv_c.
 */
inline void
alltoallv(void const*      send_buffer,
          MPI_Count const* send_counts,
          MPI_Aint const*  send_displs,
          void*            recv_buffer,
          MPI_Count const* recv_counts,
          MPI_Aint const*  recv_displs,
          MPI_Datatype     datatype)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_displs, world_size * sizeof(MPI_Aint));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_displs, world_size * sizeof(MPI_Aint));

  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const send_count = std::accumulate(send_counts, send_counts + world_size, static_cast<MPI_Count>(0)));
  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const recv_count = std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)));
  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const data_extent = extent_of(datatype));

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  DEBUG_ASSERT_GE(recv_count, 0);
  DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * data_extent);
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * data_extent);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * data_extent);

  [[maybe_unused]] auto const rc = MPI_Alltoallv_c(send_buffer, send_counts, send_displs, datatype, recv_buffer, recv_counts, recv_displs, datatype, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * data_extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Typed wrapper for MPI_Alltoallv_c.
 */
template<mpi_type_concept T>
void
alltoallv(T const* send_buffer, MPI_Count const* send_counts, MPI_Aint const* send_displs, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs)
{
  alltoallv(send_buffer, send_counts, send_displs, recv_buffer, recv_counts, recv_displs, type<T>);
}

} // namespace kaspan::mpi_basic
