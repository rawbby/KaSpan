#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <kaspan/mpi_basic/world.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief All-reduce for a single typed value.
 */
template<typename T>
auto
allreduce_single(T const& send_value, MPI_Datatype datatype, MPI_Op op) -> T
{
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);

  KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(send_value);

  T recv_value;
  KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(recv_value);

  [[maybe_unused]] auto const rc = MPI_Allreduce_c(&send_value, &recv_value, 1, datatype, op, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(recv_value);

  return recv_value;
}

/**
 * @brief All-reduce for a single typed value.
 */
template<mpi_type_concept T>
auto
allreduce_single(T const& send_value, MPI_Op op) -> T
{
  return allreduce_single(send_value, type<T>, op);
}

} // namespace kaspan::mpi_basic
