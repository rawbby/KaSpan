#pragma once

#include <util/arithmetic.hpp>
#include <util/mpi_basic.hpp>

#include <limits>

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

constexpr inline auto mpi_index_t  = mpi_basic_type<index_t>;
constexpr inline auto mpi_vertex_t = mpi_basic_type<vertex_t>;

struct Edge
{
  vertex_t u, v;
};

static constexpr auto max_edge = Edge{ std::numeric_limits<vertex_t>::max(), std::numeric_limits<vertex_t>::max() };

constexpr auto
edge_less(Edge const& lhs, Edge const& rhs) -> bool
{
  return lhs.u < rhs.u or (lhs.u == rhs.u and lhs.v < rhs.v);
}

inline MPI_Datatype mpi_edge_t = MPI_DATATYPE_NULL;

inline void
init_mpi_edge_t()
{
  DEBUG_ASSERT_EQ(mpi_edge_t, MPI_DATATYPE_NULL);
  int          blocklengths[2] = { 1, 1 };
  MPI_Aint     displs[2];
  MPI_Datatype types[2] = { mpi_vertex_t, mpi_vertex_t };

  constexpr Edge dummy{};
  MPI_Aint       base;
  MPI_Get_address(&dummy, &base);
  MPI_Get_address(&dummy.u, &displs[0]);
  MPI_Get_address(&dummy.v, &displs[1]);
  displs[0] -= base;
  displs[1] -= base;

  MPI_Type_create_struct(2, blocklengths, displs, types, &mpi_edge_t);
  MPI_Type_commit(&mpi_edge_t);

#if KASPAN_DEBUG
  MPI_Aint lb     = 0;
  MPI_Aint extent = 0;
  MPI_Type_get_extent(mpi_edge_t, &lb, &extent);
  ASSERT_EQ(extent, sizeof(Edge));
#endif
}

inline void
free_mpi_edge_t()
{
  DEBUG_ASSERT_NE(mpi_edge_t, MPI_DATATYPE_NULL);
  MPI_Type_free(&mpi_edge_t);
  mpi_edge_t = MPI_DATATYPE_NULL;
}

constexpr auto scc_id_undecided = std::numeric_limits<vertex_t>::max();
constexpr auto scc_id_singular  = scc_id_undecided - 1;

struct Degree
{
  index_t  degree_product;
  vertex_t u;
};

inline MPI_Datatype mpi_degree_t      = MPI_DATATYPE_NULL;
inline MPI_Op       mpi_degree_max_op = MPI_OP_NULL;

inline void
init_mpi_degree_t()
{
  DEBUG_ASSERT_EQ(mpi_degree_t, MPI_DATATYPE_NULL);
  int          blocklengths[2] = { 1, 1 };
  MPI_Aint     displs[2];
  MPI_Datatype types[2] = { mpi_index_t, mpi_vertex_t };

  constexpr Degree dummy{};
  MPI_Aint         base;
  MPI_Get_address(&dummy, &base);
  MPI_Get_address(&dummy.degree_product, &displs[0]);
  MPI_Get_address(&dummy.u, &displs[1]);
  displs[0] -= base;
  displs[1] -= base;

  MPI_Type_create_struct(2, blocklengths, displs, types, &mpi_degree_t);
  MPI_Type_commit(&mpi_degree_t);

#if KASPAN_DEBUG
  MPI_Aint lb     = 0;
  MPI_Aint extent = 0;
  MPI_Type_get_extent(mpi_degree_t, &lb, &extent);
  ASSERT_EQ(extent, sizeof(Degree));
#endif
}

inline void
free_mpi_degree_t()
{
  DEBUG_ASSERT_NE(mpi_degree_t, MPI_DATATYPE_NULL);
  MPI_Type_free(&mpi_degree_t);
  mpi_degree_t = MPI_DATATYPE_NULL;
}

inline void
degree_max_reduce(void* invec, void* inoutvec, int* len, MPI_Datatype* /*datatype*/)
{
  auto const* in    = static_cast<Degree const*>(invec);
  auto*       inout = static_cast<Degree*>(inoutvec);

  for (int i = 0; i < *len; ++i) {
    auto const& a = inout[i];
    auto const& b = in[i];
    if (b.degree_product > a.degree_product or
        (b.degree_product == a.degree_product and b.u > a.u)) {
      inout[i] = b;
    }
  }
}

inline void
init_mpi_degree_max_op()
{
  DEBUG_ASSERT_EQ(mpi_degree_max_op, MPI_OP_NULL);
  MPI_Op_create(&degree_max_reduce, /*commute=*/1, &mpi_degree_max_op);
}

inline void
free_mpi_degree_max_op()
{
  DEBUG_ASSERT_NE(mpi_degree_max_op, MPI_OP_NULL);
  MPI_Op_free(&mpi_degree_max_op);
  mpi_degree_max_op = MPI_OP_NULL;
}

#define KASPAN_DEFAULT_INIT() \
  MPI_DEFAULT_INIT();         \
  init_mpi_edge_t();          \
  init_mpi_degree_t();        \
  init_mpi_degree_max_op();
