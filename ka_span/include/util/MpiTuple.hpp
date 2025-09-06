#pragma once

#include <util/Arithmetic.hpp>

#include <mpi.h>
#include <tuple>
#include <type_traits>

// clang-format off
template<ArithmeticConcept T>
constexpr MPI_Datatype MpiBasicType =
  (I8Concept<T>  ? MPI_INT8_T   :
  (I16Concept<T> ? MPI_INT16_T  :
  (I32Concept<T> ? MPI_INT32_T  :
  (I64Concept<T> ? MPI_INT64_T  :
  (U8Concept<T>  ? MPI_UINT8_T  :
  (U16Concept<T> ? MPI_UINT16_T :
  (U32Concept<T> ? MPI_UINT32_T :
  (U64Concept<T> ? MPI_UINT64_T :
  (F32Concept<T> ? MPI_FLOAT    :
  (F64Concept<T> ? MPI_DOUBLE   :
  MPI_DATATYPE_NULL))))))))));
// clang-format on

template<typename T>
concept MpiBasicConcept = MpiBasicType<T> != MPI_DATATYPE_NULL;

template<MpiBasicConcept... Ts>
using MpiTupleType = std::tuple<Ts...>;

template<typename T>
concept MpiTupleConcept =
  requires { typename std::tuple_size<T>::type; } and
  (0 < std::tuple_size_v<T>) and
  ([]<size_t... Is>(std::index_sequence<Is...>) {
    return (MpiBasicConcept<std::tuple_element_t<Is, T>> and ...);
  }(std::make_index_sequence<std::tuple_size_v<T>>{}));

template<MpiTupleConcept T, size_t I = 0>
constexpr int
mpi_tuple_cmp(T const& a, T const& b)
{
  if constexpr (I == std::tuple_size_v<T>) {
    return 0;
  } else {
    if (std::get<I>(a) < std::get<I>(b))
      return -1;
    if (std::get<I>(b) < std::get<I>(a))
      return 1;
    return mpi_tuple_cmp<T, I + 1>(a, b);
  }
}

template<MpiTupleConcept T, class P>
  requires requires(P p, int a) { { p(a, a) } -> std::convertible_to<bool>; }
constexpr void
mpi_tuple_cmp_kernel(void* in, void* inout, int* len, MPI_Datatype*) // NOLINT(*-non-const-parameter)
{
  auto const* in_    = static_cast<T const*>(in);
  auto*       inout_ = static_cast<T*>(inout);

  for (int i = 0; i < *len; ++i)
    if (P{}(mpi_tuple_cmp(in_[i], inout_[i]), 0))
      inout_[i] = in_[i];
}

template<MpiTupleConcept T>
  requires(0 < std::tuple_size_v<T>)
MPI_Datatype
mpi_make_tuple_type()
{
  constexpr auto N = std::tuple_size_v<T>;

  MPI_Datatype type;
  MPI_Datatype types[N];
  int          sizes[N];
  MPI_Aint     offsets[N];

  T        dummy{};
  MPI_Aint base;
  MPI_Get_address(&dummy, &base);

  auto fill = [&]<typename I>(I) {
    constexpr size_t i = I::value;
    using element_type = std::tuple_element_t<i, T>;
    types[i]           = MpiBasicType<element_type>;
    sizes[i]           = 1;
    MPI_Aint ai;
    auto&    ref = std::get<i>(dummy);
    MPI_Get_address(&ref, &ai);
    offsets[i] = ai - base;
  };

  [&]<size_t... Is>(std::index_sequence<Is...>) {
    (fill(std::integral_constant<size_t, Is>{}), ...);
  }(std::make_index_sequence<N>{});

  MPI_Type_create_struct(static_cast<int>(N), sizes, offsets, types, &type);
  MPI_Type_commit(&type);
  return type;
}

template<MpiTupleConcept T>
MPI_Op
mpi_make_tuple_min_op()
{
  MPI_Op op;
  MPI_Op_create(&mpi_tuple_cmp_kernel<T, std::less<int> >, 1, &op);
  return op;
}

template<MpiTupleConcept T>
MPI_Op
mpi_make_tuple_max_op()
{
  MPI_Op op;
  MPI_Op_create(&mpi_tuple_cmp_kernel<T, std::greater<int>>, 1, &op);
  return op;
}

template<MpiTupleConcept T>
constexpr auto
mpi_tuple_lt(T lhs, T rhs) -> bool
{
  return std::less{}(mpi_tuple_cmp(lhs, rhs), 0);
}

template<MpiTupleConcept T>
constexpr auto
mpi_tuple_gt(T lhs, T rhs) -> bool
{
  return std::greater{}(mpi_tuple_cmp(lhs, rhs), 0);
}

template<class... Ts>
void
mpi_free(Ts&... xs)
{
  auto free_one = []<typename T>(T& obj) {
    if constexpr (std::is_same_v<T, MPI_Datatype>) {
      if (obj != MPI_DATATYPE_NULL) {
        MPI_Type_free(&obj);
        obj = MPI_DATATYPE_NULL;
      }
    }

    if constexpr (std::is_same_v<T, MPI_Op>) {
      if (obj != MPI_OP_NULL) {
        MPI_Op_free(&obj);
        obj = MPI_OP_NULL;
      }
    }
  };

  (free_one(xs), ...);
}

template<class T>
  requires MpiBasicConcept<T> or MpiTupleConcept<T>
class MpiTypeContainer final
{
public:
  MpiTypeContainer()
  {
    if constexpr (MpiTupleConcept<T>) {
      datatype_ = mpi_make_tuple_type<T>();
    }
  }

  ~MpiTypeContainer()
  {
    if constexpr (MpiBasicConcept<T>) {
      mpi_free(datatype_);
    }
  }

  MpiTypeContainer(MpiTypeContainer const&) = delete;
  MpiTypeContainer(MpiTypeContainer&&)      = default;

  auto operator=(MpiTypeContainer const&) -> MpiTypeContainer& = delete;
  auto operator=(MpiTypeContainer&&) -> MpiTypeContainer&      = default;

  [[nodiscard]] auto get() const -> MPI_Datatype
  {
    if constexpr (MpiTupleConcept<T>) {
      return datatype_;
    }
    return MpiBasicType<T>;
  }

private:
  std::conditional_t<MpiTupleConcept<T>, MPI_Datatype, std::monostate> datatype_;
};
