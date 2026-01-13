#pragma once

#include <kaspan/util/arithmetic.hpp>
#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/assert_in_range.hpp>
#include <kaspan/debug/assert_ne.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/mpi_basic/init.hpp>
#include <kaspan/mpi_basic/type.hpp>
#include <kaspan/util/pp.hpp>

#include <mpi.h>

#include <limits>

namespace kaspan {

#ifdef KASPAN_INDEX64
using index_t = i64;
#else
using index_t = i32;
#endif

#ifdef KASPAN_VERTEX64
using vertex_t = i64;
#else
using vertex_t = i32;
#endif

constexpr inline auto mpi_index_t  = mpi_basic::type<index_t>;
constexpr inline auto mpi_vertex_t = mpi_basic::type<vertex_t>;

struct edge
{
  vertex_t u, v;
};

static constexpr auto max_edge = edge{ std::numeric_limits<vertex_t>::max(), std::numeric_limits<vertex_t>::max() };

constexpr auto
edge_less(edge const& lhs, edge const& rhs) -> bool
{
  return lhs.u < rhs.u or (lhs.u == rhs.u and lhs.v < rhs.v);
}

inline mpi_basic::Datatype mpi_edge_t = mpi_basic::datatype_null;

inline void
init_mpi_edge_t()
{
  DEBUG_ASSERT_EQ(mpi_edge_t, mpi_basic::datatype_null);
  int                 blocklengths[2] = { 1, 1 };
  MPI_Aint            displs[2];
  mpi_basic::Datatype types[2] = { mpi_vertex_t, mpi_vertex_t };

  constexpr edge dummy{};
  MPI_Aint       base = 0;
  mpi_basic::get_address(&dummy, &base);
  mpi_basic::get_address(&dummy.u, &displs[0]);
  mpi_basic::get_address(&dummy.v, &displs[1]);
  displs[0] -= base;
  displs[1] -= base;

  mpi_basic::type_create_struct(2, blocklengths, displs, types, &mpi_edge_t);
  mpi_basic::type_commit(&mpi_edge_t);

  IF(KASPAN_DEBUG, MPI_Aint lb = 0; MPI_Aint extent = 0; mpi_basic::type_get_extent(mpi_edge_t, &lb, &extent); ASSERT_EQ(extent, sizeof(edge));)
}

inline void
free_mpi_edge_t()
{
  DEBUG_ASSERT_NE(mpi_edge_t, mpi_basic::datatype_null);
  mpi_basic::type_free(&mpi_edge_t);
  mpi_edge_t = mpi_basic::datatype_null;
}

constexpr auto scc_id_undecided = std::numeric_limits<vertex_t>::max();
constexpr auto scc_id_singular  = scc_id_undecided - 1;

struct degree
{
  index_t  degree_product;
  vertex_t u;
};

inline mpi_basic::Datatype mpi_degree_t      = mpi_basic::datatype_null;
inline mpi_basic::Op       mpi_degree_max_op = mpi_basic::op_null;

inline void
init_mpi_degree_t()
{
  DEBUG_ASSERT_EQ(mpi_degree_t, mpi_basic::datatype_null);
  int                 blocklengths[2] = { 1, 1 };
  MPI_Aint            displs[2];
  mpi_basic::Datatype types[2] = { mpi_index_t, mpi_vertex_t };

  constexpr degree dummy{};
  MPI_Aint         base = 0;
  mpi_basic::get_address(&dummy, &base);
  mpi_basic::get_address(&dummy.degree_product, &displs[0]);
  mpi_basic::get_address(&dummy.u, &displs[1]);
  displs[0] -= base;
  displs[1] -= base;

  mpi_basic::type_create_struct(2, blocklengths, displs, types, &mpi_degree_t);
  mpi_basic::type_commit(&mpi_degree_t);

  IF(KASPAN_DEBUG, MPI_Aint lb = 0; MPI_Aint extent = 0; mpi_basic::type_get_extent(mpi_degree_t, &lb, &extent); ASSERT_EQ(extent, sizeof(degree));)
}

inline void
free_mpi_degree_t()
{
  DEBUG_ASSERT_NE(mpi_degree_t, mpi_basic::datatype_null);
  mpi_basic::type_free(&mpi_degree_t);
  mpi_degree_t = mpi_basic::datatype_null;
}

inline void
degree_max_reduce(void* invec, void* inoutvec, int* len, mpi_basic::Datatype* /*datatype*/) // NOLINT(readability-non-const-parameter)
{
  auto const* in    = static_cast<degree const*>(invec);
  auto*       inout = static_cast<degree*>(inoutvec);

  for (int i = 0; i < *len; ++i) {
    auto const& a = inout[i];
    auto const& b = in[i];
    if (b.degree_product > a.degree_product or (b.degree_product == a.degree_product and b.u > a.u)) {
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
