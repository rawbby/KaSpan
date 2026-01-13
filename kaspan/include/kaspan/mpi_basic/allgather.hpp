#pragma once

#include <kaspan/debug/assert_true.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/extent_of.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/debug/assert_ge.hpp>
#include <kaspan/debug/assert_ne.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/debug/assert_eq.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief Gathers data from all processes and distributes it to all processes (overload with specific MPI_Datatype).
 *
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param send_type The MPI datatype of the send buffer.
 * @param recv_buffer The buffer to receive the data.
 * @param recv_count The number of elements to receive from each process.
 * @param recv_type The MPI datatype of the receive buffer.
 */
inline void
allgather(void const* send_buffer, MPI_Count send_count, MPI_Datatype send_type, void* recv_buffer, MPI_Count recv_count, MPI_Datatype recv_type)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(send_type, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_type, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  DEBUG_ASSERT_GE(recv_count, 0);
  DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const send_extent = extent_of(send_type));
  IF(KASPAN_MEMCHECK, auto const recv_extent = extent_of(recv_type));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * send_extent);
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * world_size * recv_extent);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * world_size * recv_extent);

  [[maybe_unused]] auto const rc = MPI_Allgather_c(send_buffer, send_count, send_type, recv_buffer, recv_count, recv_type, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * world_size * recv_extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Gathers data from all processes and distributes it to all processes.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data.
 * @param recv_count The number of elements to receive from each process.
 */
template<mpi_type_concept T>
void
allgather(T const* send_buffer, MPI_Count send_count, T* recv_buffer, MPI_Count recv_count)
{
  allgather(send_buffer, send_count, type<T>, recv_buffer, recv_count, type<T>);
}

/**
 * @brief Gathers data from all processes and distributes it to all processes (overload for single elements).
 *
 * @tparam T The type of the data to gather.
 * @param send_value The value to send.
 * @param recv_buffer The buffer to receive the data.
 */
template<mpi_type_concept T>
void
allgather(T const& send_value, T* recv_buffer)
{
  allgather(&send_value, 1, type<T>, recv_buffer, 1, type<T>);
}

} // namespace kaspan::mpi_basic
