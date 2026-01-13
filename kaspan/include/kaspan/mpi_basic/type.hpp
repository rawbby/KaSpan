#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <mpi.h>

namespace kaspan::mpi_basic {

// clang-format off
/**
 * @brief Mapping from C++ types to MPI datatypes.
 */
template<typename T>
constexpr inline auto type =
  byte_concept<T> ? MPI_BYTE     :
  i8_concept<T>   ? MPI_INT8_T   :
  i16_concept<T>  ? MPI_INT16_T  :
  i32_concept<T>  ? MPI_INT32_T  :
  i64_concept<T>  ? MPI_INT64_T  :
  u8_concept<T>   ? MPI_UINT8_T  :
  u16_concept<T>  ? MPI_UINT16_T :
  u32_concept<T>  ? MPI_UINT32_T :
  u64_concept<T>  ? MPI_UINT64_T :
  f32_concept<T>  ? MPI_FLOAT    :
  f64_concept<T>  ? MPI_DOUBLE   :
  MPI_DATATYPE_NULL;
// clang-format on

/**
 * @brief mpi_type_concept for types that have a corresponding MPI datatype.
 */
template<typename T>
concept mpi_type_concept = type<T> != MPI_DATATYPE_NULL;

constexpr inline auto byte_t = MPI_BYTE;
constexpr inline auto i8_t   = MPI_INT8_T;
constexpr inline auto i16_t  = MPI_INT16_T;
constexpr inline auto i32_t  = MPI_INT32_T;
constexpr inline auto i64_t  = MPI_INT64_T;
constexpr inline auto u8_t   = MPI_UINT8_T;
constexpr inline auto u16_t  = MPI_UINT16_T;
constexpr inline auto u32_t  = MPI_UINT32_T;
constexpr inline auto u64_t  = MPI_UINT64_T;

using Datatype = MPI_Datatype;
using Op       = MPI_Op;
using Request  = MPI_Request;

constexpr inline Datatype datatype_null = MPI_DATATYPE_NULL;
constexpr inline Op       op_null       = MPI_OP_NULL;
constexpr inline Request  request_null  = MPI_REQUEST_NULL;

constexpr inline Op sum  = MPI_SUM;
constexpr inline Op min  = MPI_MIN;
constexpr inline Op max  = MPI_MAX;
constexpr inline Op lor  = MPI_LOR;
constexpr inline Op land = MPI_LAND;

inline void
get_address(void const* location, MPI_Aint* address)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Get_address(location, address);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_create_struct(int count, int const* blocklengths, MPI_Aint const* displs, Datatype const* types, Datatype* newtype)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_create_struct(count, blocklengths, displs, types, newtype);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_commit(Datatype* type)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_commit(type);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_free(Datatype* type)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_free(type);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_get_extent(Datatype type, MPI_Aint* lb, MPI_Aint* extent)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_get_extent(type, lb, extent);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
op_create(MPI_User_function* user_fn, int commute, Op* op)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Op_create(user_fn, commute, op);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
op_free(Op* op)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Op_free(op);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

} // namespace kaspan::mpi_basic
