#pragma once

#include <kaspan/mpi_basic/extent_of.hpp>
#include <kaspan/mpi_basic/type.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief In-place all-reduce for a buffer of untyped elements.
 */
inline void
allreduce_inplace(void* send_buffer, MPI_Count send_count, MPI_Datatype datatype, MPI_Op op)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const extent = extent_of(datatype));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * extent);

  [[maybe_unused]] auto const rc = MPI_Allreduce_c(MPI_IN_PLACE, send_buffer, send_count, datatype, op, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief In-place all-reduce for a buffer of typed elements.
 */
template<mpi_type_concept T>
void
allreduce_inplace(T* send_buffer, MPI_Count send_count, MPI_Op op)
{
  allreduce_inplace(send_buffer, send_count, type<T>, op);
}

/**
 * @brief In-place all-reduce for a single typed value.
 */
template<mpi_type_concept T>
void
allreduce_inplace(T& send_value, MPI_Op op)
{
  allreduce_inplace(&send_value, 1, type<T>, op);
}

} // namespace kaspan::mpi_basic
