#pragma once

#include <mpi.h>
#include <mpi_basic/type.hpp>

namespace mpi_basic {

/**
 * @brief All-reduce for a single typed value.
 */
template<Concept T>
auto
allreduce_single(T const& send_value, MPI_Op op) -> T
{
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);
  T recv_value;
  MPI_Allreduce_c(&send_value, &recv_value, 1, type<T>, op, MPI_COMM_WORLD);
  return recv_value;
}

/**
 * @brief All-reduce for a single untyped value.
 */
template<typename T>
auto
allreduce_single(T const& send_value, MPI_Datatype datatype, MPI_Op op) -> T
{
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);
  T recv_value{};
  MPI_Allreduce_c(&send_value, &recv_value, 1, datatype, op, MPI_COMM_WORLD);
  return recv_value;
}

} // namespace mpi_basic
