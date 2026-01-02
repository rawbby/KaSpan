#pragma once

#include <mpi.h>
#include <mpi_basic/type.hpp>

namespace mpi_basic {

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
template<typename T>
void
gatherv(T const* send_buffer, int send_count, T* recv_buffer, int const* recv_counts, int const* displs, int root = 0)
{
  MPI_Gatherv(send_buffer, send_count, type<T>, recv_buffer, recv_counts, displs, type<T>, root, MPI_COMM_WORLD);
}

/**
 * @brief Gathers data with varying counts (MPI_Count/MPI_Aint) from all processes to the root process.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param recv_counts The number of elements to receive from each process (significant only at root).
 * @param displs The displacements in the receive buffer (significant only at root).
 * @param root The rank of the root process.
 */
template<typename T>
void
gatherv(T const* send_buffer, MPI_Count send_count, T* recv_buffer, MPI_Count const* recv_counts, MPI_Aint const* displs, int root = 0)
{
  MPI_Gatherv_c(send_buffer, send_count, type<T>, recv_buffer, recv_counts, displs, type<T>, root, MPI_COMM_WORLD);
}

} // namespace mpi_basic
