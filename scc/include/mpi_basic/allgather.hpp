#pragma once

#include <mpi.h>
#include <mpi_basic/type.hpp>

namespace mpi_basic {

/**
 * @brief Gathers data from all processes and distributes it to all processes.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data.
 * @param recv_count The number of elements to receive from each process.
 */
template<typename T>
void
allgather(T const* send_buffer, int send_count, T* recv_buffer, int recv_count)
{
  MPI_Allgather(send_buffer, send_count, type<T>, recv_buffer, recv_count, type<T>, MPI_COMM_WORLD);
}

/**
 * @brief Gathers data from all processes and distributes it to all processes (overload for single elements).
 *
 * @tparam T The type of the data to gather.
 * @param send_value The value to send.
 * @param recv_buffer The buffer to receive the data.
 */
template<typename T>
void
allgather(T const& send_value, T* recv_buffer)
{
  MPI_Allgather(&send_value, 1, type<T>, recv_buffer, 1, type<T>, MPI_COMM_WORLD);
}

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
allgather(void const* send_buffer, int send_count, MPI_Datatype send_type, void* recv_buffer, int recv_count, MPI_Datatype recv_type)
{
  MPI_Allgather(send_buffer, send_count, send_type, recv_buffer, recv_count, recv_type, MPI_COMM_WORLD);
}

} // namespace mpi_basic
