#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/type.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

inline auto
extent_of(MPI_Datatype datatype) -> MPI_Aint
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  MPI_Aint lb;
  MPI_Aint extent;

  KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(lb);
  KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(extent);

  [[maybe_unused]] auto const rc0 = MPI_Type_get_extent(datatype, &lb, &extent);
  DEBUG_ASSERT_EQ(rc0, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(extent);
  DEBUG_ASSERT_GE(lb, 0);
  DEBUG_ASSERT_GT(extent, 0);

  if constexpr (KASPAN_DEBUG) {
    int type_size;

    KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(type_size);

    auto const rc1 = MPI_Type_size(datatype, &type_size);
    ASSERT_EQ(rc1, MPI_SUCCESS);

    KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(type_size);
    ASSERT_EQ(type_size, extent);
  }

  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  return extent;
}

template<mpi_type_concept T>
auto
extent_of() -> MPI_Aint
{
  return extent_of(type<T>);
}

}
