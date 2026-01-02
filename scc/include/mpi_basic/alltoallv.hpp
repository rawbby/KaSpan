#pragma once

#include <mpi.h>
#include <mpi_basic/type.hpp>

namespace mpi_basic {

/**
 * @brief Typed wrapper for MPI_Alltoallv_c.
 */
template<Concept T>
void
alltoallv(T const* send_buffer, MPI_Count const* send_counts, MPI_Aint const* send_displs, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs)
{
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);
  if (send_buffer == nullptr) { DEBUG_ASSERT_EQ(std::accumulate(send_counts, send_counts + world_size, static_cast<MPI_Count>(0)), 0); }
  if (recv_buffer == nullptr) { DEBUG_ASSERT_EQ(std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)), 0); }
  MPI_Alltoallv_c(send_buffer, send_counts, send_displs, type<T>, recv_buffer, recv_counts, recv_displs, type<T>, MPI_COMM_WORLD);
}

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
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);
  if (send_buffer == nullptr) { DEBUG_ASSERT_EQ(std::accumulate(send_counts, send_counts + world_size, static_cast<MPI_Count>(0)), 0); }
  if (recv_buffer == nullptr) { DEBUG_ASSERT_EQ(std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)), 0); }
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  MPI_Alltoallv_c(send_buffer, send_counts, send_displs, datatype, recv_buffer, recv_counts, recv_displs, datatype, MPI_COMM_WORLD);
}

} // namespace mpi_basic
