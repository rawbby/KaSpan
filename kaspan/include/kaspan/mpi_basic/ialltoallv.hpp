#pragma once

#include <mpi.h>
#include <numeric> // std::accumulate

namespace kaspan::mpi_basic {

/**
 * @brief Non-blocking untyped all-to-all exchange.
 */
inline auto
ialltoallv(void const*      send_buffer,
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
  if (send_buffer == nullptr) {
    DEBUG_ASSERT_EQ(std::accumulate(send_counts, send_counts + world_size, static_cast<MPI_Count>(0)), 0);
  }
  if (recv_buffer == nullptr) {
    DEBUG_ASSERT_EQ(std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)), 0);
  }
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  MPI_Request request = MPI_REQUEST_NULL;
  MPI_Ialltoallv_c(send_buffer, send_counts, send_displs, datatype, recv_buffer, recv_counts, recv_displs, datatype, MPI_COMM_WORLD, &request);
  return request;
}

} // namespace kaspan::mpi_basic
