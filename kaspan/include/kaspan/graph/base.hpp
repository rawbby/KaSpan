#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>

#include <kamping/mpi_datatype.hpp>
#include <mpi.h>

#include <cstring>
#include <limits>

namespace kaspan {

#ifdef KASPAN_INDEX64
using index_t = u64;
#else
using index_t = u32;
#endif

#ifdef KASPAN_VERTEX64
using vertex_t = u64;
#else
using vertex_t = u32;
#endif

constexpr inline auto mpi_index_t  = mpi_basic::type<index_t>;
constexpr inline auto mpi_vertex_t = mpi_basic::type<vertex_t>;

struct edge_t
{
  vertex_t u = 0;
  vertex_t v = 0;

  constexpr auto operator<(
    edge_t e) const noexcept -> bool
  {
    return u < e.u | u == e.u & v < e.v;
  }
};
static_assert(std::is_trivially_copyable_v<edge_t>);
static_assert(std::is_trivially_destructible_v<edge_t>);

static constexpr auto max_edge = edge_t{ std::numeric_limits<vertex_t>::max(), std::numeric_limits<vertex_t>::max() };

constexpr auto
edge_less(
  edge_t const& lhs,
  edge_t const& rhs) -> bool
{
  return lhs.u < rhs.u || (lhs.u == rhs.u && lhs.v < rhs.v);
}

inline mpi_basic::Datatype mpi_edge_t = mpi_basic::datatype_null;

inline void
init_mpi_edge_t()
{
  DEBUG_ASSERT_EQ(mpi_edge_t, mpi_basic::datatype_null);

  constexpr int      count = 2;
  constexpr MPI_Aint displ = 0;
  MPI_Datatype const type  = mpi_vertex_t;

  mpi_basic::type_create_struct(1, &count, &displ, &type, &mpi_edge_t);
  mpi_basic::type_commit(&mpi_edge_t);
}

inline void
free_mpi_edge_t()
{
  DEBUG_ASSERT_NE(mpi_edge_t, mpi_basic::datatype_null);
  mpi_basic::type_free(&mpi_edge_t);
  mpi_edge_t = mpi_basic::datatype_null;
}

} // namespace kaspan

namespace kamping {
template<>
struct mpi_type_traits<kaspan::edge_t>
{
  static constexpr bool has_to_be_committed = false;
  static MPI_Datatype   data_type()
  {
    return kaspan::mpi_edge_t;
  }
};
}

namespace kaspan {

constexpr auto scc_id_undecided = std::numeric_limits<vertex_t>::max();
constexpr auto scc_id_singular  = scc_id_undecided - 1;

struct degree_t
{
  using common_t = std::conditional_t<(sizeof(index_t) > sizeof(vertex_t)), index_t, vertex_t>;

  common_t degree_product = 0;
  common_t u              = 0;
};

inline mpi_basic::Datatype mpi_degree_t      = mpi_basic::datatype_null;
inline mpi_basic::Op       mpi_degree_max_op = mpi_basic::op_null;

inline void
init_mpi_degree_t()
{
  DEBUG_ASSERT_EQ(mpi_degree_t, mpi_basic::datatype_null);

  constexpr int      count = 2;
  constexpr MPI_Aint displ = 0;
  MPI_Datatype const type  = mpi_basic::type<degree_t::common_t>;

  mpi_basic::type_create_struct(1, &count, &displ, &type, &mpi_degree_t);
  mpi_basic::type_commit(&mpi_degree_t);
}

inline void
free_mpi_degree_t()
{
  DEBUG_ASSERT_NE(mpi_degree_t, mpi_basic::datatype_null);
  mpi_basic::type_free(&mpi_degree_t);
  mpi_degree_t = mpi_basic::datatype_null;
}

inline void
degree_max_reduce(
  void* invec,
  void* inoutvec,
  int*  len,
  mpi_basic::Datatype* /*datatype*/) // NOLINT(readability-non-const-parameter)
{
  auto const* in    = static_cast<degree_t const*>(invec);
  auto*       inout = static_cast<degree_t*>(inoutvec);

  for (int i = 0; i < *len; ++i) {
    auto const& a = inout[i];
    auto const& b = in[i];
    if (b.degree_product > a.degree_product || (b.degree_product == a.degree_product && b.u > a.u)) {
      inout[i] = b;
    }
  }
}

inline void
init_mpi_degree_max_op()
{
  DEBUG_ASSERT_EQ(mpi_degree_max_op, mpi_basic::op_null);
  mpi_basic::op_create(&degree_max_reduce, /*commute=*/1, &mpi_degree_max_op);
}

inline void
free_mpi_degree_max_op()
{
  DEBUG_ASSERT_NE(mpi_degree_max_op, mpi_basic::op_null);
  mpi_basic::op_free(&mpi_degree_max_op);
  mpi_degree_max_op = mpi_basic::op_null;
}

#define SCC_ID_UNDECIDED_FILTER(LOCAL_N, SCC_ID)                                                                                                                                   \
  [=](auto&& k) {                                                                                                                                                                  \
    DEBUG_ASSERT_IN_RANGE(k, 0, LOCAL_N);                                                                                                                                          \
    return (SCC_ID)[k] == scc_id_undecided;                                                                                                                                        \
  }

#define KASPAN_DEFAULT_INIT()                                                                                                                                                      \
  MPI_INIT();                                                                                                                                                                      \
  kaspan::init_mpi_edge_t();                                                                                                                                                       \
  kaspan::init_mpi_degree_t();                                                                                                                                                     \
  kaspan::init_mpi_degree_max_op();

} // namespace kaspan
