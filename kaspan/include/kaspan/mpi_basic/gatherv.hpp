#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/extent_of.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <kaspan/mpi_basic/world.hpp>

#include <mpi.h>
#include <numeric> // std::accumulate

namespace kaspan::mpi_basic {

/**
 * @brief Gathers data with varying counts from all processes to the root process (untyped).
 */
inline void
gatherv(void const*      send_buffer,
        MPI_Count        send_count,
        MPI_Datatype     send_type,
        void*            recv_buffer,
        MPI_Count const* recv_counts,
        MPI_Aint const*  displs,
        MPI_Datatype     recv_type,
        int              root = 0)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(send_type, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_type, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const send_extent = extent_of(send_type));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * send_extent);

  IF(KASPAN_MEMCHECK, auto const recv_extent = extent_of(recv_type));

  if (world_rank == root) {
    DEBUG_ASSERT_NE(recv_counts, nullptr);
    DEBUG_ASSERT_NE(displs, nullptr);

    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(displs, world_size * sizeof(MPI_Aint));

    IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const recv_count = std::accumulate(recv_counts, recv_counts + world_size, static_cast<MPI_Count>(0)));

    DEBUG_ASSERT_GE(recv_count, 0);
    DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

    KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * recv_extent);
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * recv_extent);

    [[maybe_unused]] auto const rc = MPI_Gatherv_c(send_buffer, send_count, send_type, recv_buffer, recv_counts, displs, recv_type, root, comm_world);
    DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * recv_extent);
  } else {
    [[maybe_unused]] auto const rc = MPI_Gatherv_c(send_buffer, send_count, send_type, nullptr, nullptr, nullptr, recv_type, root, comm_world);
    DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  }
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Gathers data with varying counts from all processes to the root process.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param recv_counts The number of elements to receive from each process (significant only at root).
 * @param displs The displacements in the receive buffer (significant only at root).
 * @param root The rank of the root process.
 */
template<mpi_type_concept T>
void
gatherv(T const* send_buffer, MPI_Count send_count, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* displs, int root = 0)
{
  gatherv(send_buffer, send_count, type<T>, recv_buffer, recv_counts, displs, type<T>, root);
}

} // namespace kaspan::mpi_basic
