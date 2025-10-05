#pragma once

#include <graph/AllGatherGraph.hpp>
#include <graph/GraphPart.hpp>
#include <graph/Part.hpp>
#include <kagen.h>
#include <kamping/communicator.hpp>
#include <util/Result.hpp>

inline auto
KaGenGraphPart(kamping::Communicator<>& comm, char const* generator_args) -> Result<GraphPart<ExplicitSortedContinuousWorldPart>>
{
  using Part = ExplicitSortedContinuousWorldPart;

  kagen::KaGen gen(MPI_COMM_WORLD);
  gen.UseCSRRepresentation();

  auto       ka_graph   = gen.GenerateFromOptionString(generator_args);
  auto const n          = ka_graph.NumberOfGlobalVertices();
  auto const m          = ka_graph.NumberOfGlobalEdges();
  auto const local_n    = ka_graph.NumberOfLocalVertices();
  auto const local_m    = ka_graph.NumberOfLocalEdges();
  auto const [beg, end] = ka_graph.vertex_range;
  auto const rank       = comm.rank();
  auto const size       = comm.size();

  GraphPart<Part> g;
  RESULT_TRY(g.part, Part::create(n, beg, end, rank, size));

  g.n = n;
  g.m = m;

  RESULT_TRY(g.fw_head, IndexBuffer::create(local_n + 1));
  for (vertex_t k = 0; k <= local_n; ++k) {
    g.fw_head[k] = ka_graph.xadj[k];
  }

  RESULT_TRY(g.fw_csr, VertexBuffer::create(local_m));
  for (index_t it = 0; it < local_m; ++it) {
    g.fw_csr[it] = ka_graph.adjncy[it];
  }

  ka_graph.FreeCSR();
  ComplementBackwardsGraph<Part>(g);

  return g;
}
