#pragma once

#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/mpi_basic/allgather_counts.hpp>
#include <kaspan/mpi_basic/counts_and_displs.hpp>
#include <kaspan/mpi_basic/displs.hpp>
#include <kaspan/mpi_basic/extent_of.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/assert_ge.hpp>
#include <kaspan/debug/assert_ne.hpp>
#include <kaspan/debug/assert_true.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/util/pp.hpp>

#include <mpi.h>
#include <numeric>

namespace kaspan::mpi_basic {

/**
 * @brief Untyped wrapper for MPI_Allgatherv_c.
 */
inline void
allgatherv(void const* send_buffer, MPI_Count send_count, void* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs, MPI_Datatype datatype)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_displs, world_size * sizeof(MPI_Aint));

  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const recv_count = std::accumulate(recv_counts, recv_counts + world_size, integral_cast<MPI_Count>(0)));
  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const extent = extent_of(datatype));

  DEBUG_ASSERT_GE(recv_count, 0);
  DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * extent);
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * extent);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * extent);

  [[maybe_unused]] auto const rc = MPI_Allgatherv_c(send_buffer, send_count, datatype, recv_buffer, recv_counts, recv_displs, datatype, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Typed wrapper for MPI_Allgatherv_c with element counts and displacements.
 *
 * @param send_buffer Pointer to local elements.
 * @param send_count Number of local elements.
 * @param recv_buffer Pointer to receive buffer holding sum(counts) elements.
 * @param recv_counts Per-rank element counts (MPI_Count[world_size]).
 * @param recv_displs Per-rank displacements (MPI_Aint[world_size]).
 */
template<mpi_type_concept T>
void
allgatherv(T const* send_buffer, MPI_Count send_count, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* recv_displs)
{
  allgatherv(send_buffer, send_count, recv_buffer, recv_counts, recv_displs, type<T>);
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
template<mpi_type_concept T>
auto
allgatherv(T const* send_buffer, MPI_Count send_count)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  auto [buffer, c, d] = counts_and_displs();
  allgather_counts(send_count, c);

  auto const count = displs(c, d);
  auto       recv  = make_array<T>(count);

  allgatherv<T>(send_buffer, send_count, recv.data(), c, d);
  return PACK(recv, count);
}

} // namespace kaspan::mpi_basic
