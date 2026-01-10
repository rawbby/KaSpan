#pragma once

#include <kaspan/mpi_basic/type.hpp>
#include <mpi.h>
#include <numeric> // std::accumulate

namespace kaspan::mpi_basic {

/**
 * @brief Typed wrapper for MPI_Alltoallv_c.
 */
template<mpi_type_concept T>
void
alltoallv(T const* send_buffer, MPI_Count const* send_counts, MPI_Aint const* send_displs, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs)
{
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);
  // Some MPI implementations (e.g., Intel MPI) do not accept nullptr buffer pointers.
  // Provide a properly aligned stack-local dummy buffer.
  alignas(T) char dummy_storage[sizeof(T)];
  T*       dummy_ptr          = reinterpret_cast<T*>(dummy_storage);
  T const* actual_send_buffer = (send_buffer == nullptr) ? dummy_ptr : send_buffer;
  T*       actual_recv_buffer = (recv_buffer == nullptr) ? dummy_ptr : recv_buffer;
  MPI_Alltoallv_c(actual_send_buffer, send_counts, send_displs, type<T>, actual_recv_buffer, recv_counts, recv_displs, type<T>, MPI_COMM_WORLD);
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
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  // Some MPI implementations (e.g., Intel MPI) do not accept nullptr buffer pointers.
  // Provide a properly aligned stack-local dummy buffer.
  alignas(std::max_align_t) char dummy_storage[64];
  void*       dummy_ptr          = static_cast<void*>(dummy_storage);
  void const* actual_send_buffer = (send_buffer == nullptr) ? dummy_ptr : send_buffer;
  void*       actual_recv_buffer = (recv_buffer == nullptr) ? dummy_ptr : recv_buffer;
  MPI_Alltoallv_c(actual_send_buffer, send_counts, send_displs, datatype, actual_recv_buffer, recv_counts, recv_displs, datatype, MPI_COMM_WORLD);
}

} // namespace kaspan::mpi_basic
