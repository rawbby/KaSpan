#pragma once

#include <mpi.h>
#include <util/arithmetic.hpp>

namespace mpi_basic {

// clang-format off
/**
 * @brief Mapping from C++ types to MPI datatypes.
 */
template<typename T>
constexpr inline auto type =
  ByteConcept<T> ? MPI_BYTE     :
  I8Concept<T>   ? MPI_INT8_T   :
  I16Concept<T>  ? MPI_INT16_T  :
  I32Concept<T>  ? MPI_INT32_T  :
  I64Concept<T>  ? MPI_INT64_T  :
  U8Concept<T>   ? MPI_UINT8_T  :
  U16Concept<T>  ? MPI_UINT16_T :
  U32Concept<T>  ? MPI_UINT32_T :
  U64Concept<T>  ? MPI_UINT64_T :
  F32Concept<T>  ? MPI_FLOAT    :
  F64Concept<T>  ? MPI_DOUBLE   :
  MPI_DATATYPE_NULL;
// clang-format on

/**
 * @brief Concept for types that have a corresponding MPI datatype.
 */
template<typename T>
concept Concept = type<T> != MPI_DATATYPE_NULL;

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
  MPI_Get_address(location, address);
}

inline void
type_create_struct(int count, int const* blocklengths, MPI_Aint const* displs, Datatype const* types, Datatype* newtype)
{
  MPI_Type_create_struct(count, blocklengths, displs, types, newtype);
}

inline void
type_commit(Datatype* type)
{
  MPI_Type_commit(type);
}

inline void
type_free(Datatype* type)
{
  MPI_Type_free(type);
}

inline void
type_get_extent(Datatype type, MPI_Aint* lb, MPI_Aint* extent)
{
  MPI_Type_get_extent(type, lb, extent);
}

inline void
op_create(MPI_User_function* user_fn, int commute, Op* op)
{
  MPI_Op_create(user_fn, commute, op);
}

inline void
op_free(Op* op)
{
  MPI_Op_free(op);
}

} // namespace mpi_basic
