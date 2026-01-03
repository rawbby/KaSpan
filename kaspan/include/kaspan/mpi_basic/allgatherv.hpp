#pragma once

#include <kaspan/memory/borrow.hpp>
#include <kaspan/mpi_basic/allgatherv_counts.hpp>
#include <kaspan/mpi_basic/counts_and_displs.hpp>
#include <kaspan/mpi_basic/displs.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <mpi.h>
#include <numeric> // std::accumulate

namespace kaspan::mpi_basic {

/**
 * @brief Typed wrapper for MPI_Allgatherv_c with element counts and displacements.
 *
 * @param send_buffer Pointer to local elements.
 * @param send_count Number of local elements.
 * @param recv_buffer Pointer to receive buffer holding sum(counts) elements.
 * @param recv_counts Per-rank element counts (MPI_Count[world_size]).
 * @param recv_displs Per-rank displacements (MPI_Aint[world_size]).
 */
template<Concept T>
void
allgatherv(T const* send_buffer, MPI_Count send_count, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 or send_buffer != nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);
  if (recv_buffer == nullptr) {
    DEBUG_ASSERT_EQ(std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)), 0);
  }
  MPI_Allgatherv_c(send_buffer, send_count, type<T>, recv_buffer, recv_counts, recv_displs, type<T>, MPI_COMM_WORLD);
}

/**
 * @brief Untyped wrapper for MPI_Allgatherv_c.
 */
inline void
allgatherv(void const* send_buffer, MPI_Count send_count, void* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs, MPI_Datatype datatype)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 or send_buffer != nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  if (recv_buffer == nullptr) {
    DEBUG_ASSERT_EQ(std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)), 0);
  }
  MPI_Allgatherv_c(send_buffer, send_count, datatype, recv_buffer, recv_counts, recv_displs, datatype, MPI_COMM_WORLD);
}

/**
 * @brief Allocate-and-call convenience wrapper for MPI_Allgatherv_c.
 *
 * @param send_buffer Pointer to local elements.
 * @param send_count Number of local elements.
 * @return { buffer, recv, count } where:
 *         - buffer owns the receive storage (count elements),
 *         - recv points into buffer,
 *         - count is the total number of received elements.
 */
template<Concept T>
auto
allgatherv(T const* send_buffer, MPI_Count send_count)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 or send_buffer != nullptr);

  struct result
  {
    buffer    storage;
    T*        recv;
    MPI_Count count{};
  };

  result res;
  auto [buffer, counts, d] = counts_and_displs();
  allgatherv_counts(send_count, counts);

  res.count   = displs(counts, d);
  res.storage = make_buffer<T>(res.count);
  res.recv    = static_cast<T*>(res.storage.data());

  allgatherv<T>(send_buffer, send_count, res.recv, counts, d);
  return res;
}

} // namespace kaspan::mpi_basic
