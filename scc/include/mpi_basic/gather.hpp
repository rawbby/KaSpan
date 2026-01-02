#pragma once

#include <mpi_basic/type.hpp>
#include <mpi_basic/world.hpp>
#include <mpi.h>

namespace mpi_basic {

/**
 * @brief Gathers data from all processes to the root process.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param recv_count The number of elements to receive from each process (significant only at root).
 * @param root The rank of the root process.
 */
template<typename T>
void
gather(T const* send_buffer, int send_count, T* recv_buffer, int recv_count, int root = 0)
{
  MPI_Gather(send_buffer, send_count, type<T>, recv_buffer, recv_count, type<T>, root, MPI_COMM_WORLD);
}

/**
 * @brief Gathers data from all processes to the root process (overload for single elements).
 *
 * @tparam T The type of the data to gather.
 * @param send_value The value to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param root The rank of the root process.
 */
template<typename T>
void
gather(T const& send_value, T* recv_buffer, int root = 0)
{
  MPI_Gather(&send_value, 1, type<T>, recv_buffer, 1, type<T>, root, MPI_COMM_WORLD);
}

} // namespace mpi_basic
