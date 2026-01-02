#pragma once

#include <mpi.h>
#include <mpi_basic/type.hpp>
#include <util/arithmetic.hpp>

namespace mpi_basic {

/**
 * @brief In-place all-reduce for a buffer of typed elements.
 */
template<Concept T>
void
allreduce_inplace(T* send_buffer, MPI_Count send_count, MPI_Op op)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 or send_buffer != nullptr);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);
  MPI_Allreduce_c(MPI_IN_PLACE, send_buffer, send_count, type<T>, op, MPI_COMM_WORLD);
}

/**
 * @brief In-place all-reduce for a single typed value.
 */
template<Concept T>
void
allreduce_inplace(T& send_value, MPI_Count send_count, MPI_Op op)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);
  MPI_Allreduce_c(MPI_IN_PLACE, &send_value, send_count, type<T>, op, MPI_COMM_WORLD);
}

/**
 * @brief In-place all-reduce for a buffer of untyped elements.
 */
template<typename T>
void
allreduce_inplace(T* send_buffer, MPI_Count send_count, MPI_Datatype datatype, MPI_Op op)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 or send_buffer != nullptr);
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);
  MPI_Allreduce_c(MPI_IN_PLACE, send_buffer, send_count, datatype, op, MPI_COMM_WORLD);
}

/**
 * @brief In-place all-reduce for a single untyped value.
 */
template<typename T>
void
allreduce_inplace(T& send_value, MPI_Count send_count, MPI_Datatype datatype, MPI_Op op)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);
  MPI_Allreduce_c(MPI_IN_PLACE, &send_value, send_count, datatype, op, MPI_COMM_WORLD);
}

} // namespace mpi_basic
