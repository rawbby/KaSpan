#pragma once

#include <util/Arithmetic.hpp>

#include <mpi.h>

// clang-format off
template<typename T>
constexpr inline MPI_Datatype mpi_basic_type =
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

template<typename T>
concept MpiBasicConcept = mpi_basic_type<T> != MPI_DATATYPE_NULL;
