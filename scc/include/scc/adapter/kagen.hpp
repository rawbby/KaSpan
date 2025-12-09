#pragma once

#include <kagen.h>
#include <memory/buffer.hpp>
#include <scc/backward_complement.hpp>
#include <scc/part.hpp>

inline auto
kagen_forward_graph_part(char const* generator_args)
{
  struct
  {
    using Part = ExplicitSortedContinuousWorldPart;
    Buffer    buffer;
    Part      part;
    index_t   m;
    index_t   local_m;
    index_t*  head;
    vertex_t* csr;
  } result;

  kagen::KaGen graph_generator{ MPI_COMM_WORLD };
  graph_generator.UseCSRRepresentation();

  auto const ka_graph   = graph_generator.GenerateFromOptionString(generator_args);
  auto const global_n   = static_cast<vertex_t>(ka_graph.NumberOfGlobalVertices());
  auto const global_m   = static_cast<index_t>(ka_graph.NumberOfGlobalEdges());
  auto const local_n    = static_cast<vertex_t>(ka_graph.NumberOfLocalVertices());
  auto const local_fw_m = static_cast<index_t>(ka_graph.NumberOfLocalEdges());
  auto const vertex_end = static_cast<vertex_t>(ka_graph.vertex_range.second);

  result.buffer = Buffer{
    page_ceil<i32>(mpi_world_size),
    page_ceil<index_t>(local_n + 1),
    page_ceil<vertex_t>(local_fw_m)
  };
  void* memory = result.buffer.data();

  result.part = ExplicitSortedContinuousWorldPart{ global_n, vertex_end, mpi_world_rank, mpi_world_size, memory };
  DEBUG_ASSERT_EQ(result.part.n, global_n);
  DEBUG_ASSERT_EQ(result.part.local_n(), local_n);

  result.m       = global_m;
  result.local_m = local_fw_m;
  result.head    = borrow<index_t>(memory, local_n + 1);
  result.csr     = borrow<vertex_t>(memory, local_fw_m);

  for (vertex_t k = 0; k <= local_n; ++k) {
    DEBUG_ASSERT_GE(ka_graph.xadj[k], 0, "k={}", k);
    DEBUG_ASSERT_LE(ka_graph.xadj[k], local_fw_m, "k={}", k);
    result.head[k] = static_cast<index_t>(ka_graph.xadj[k]);
    DEBUG_ASSERT_GE(result.head[k], 0, "k={}", k);
    DEBUG_ASSERT_LE(result.head[k], local_fw_m, "k={}", k);
  }
  for (index_t it = 0; it < local_fw_m; ++it) {
    DEBUG_ASSERT_GE(ka_graph.adjncy[it], 0);
    DEBUG_ASSERT_LT(ka_graph.adjncy[it], global_n);
    result.csr[it] = static_cast<vertex_t>(ka_graph.adjncy[it]);
    DEBUG_ASSERT_GE(result.csr[it], 0);
    DEBUG_ASSERT_LT(result.csr[it], global_n);
  }

  DEBUG_ASSERT_VALID_GRAPH_PART(result.part, result.head, result.csr);
  return result;
}

inline auto
kagen_graph_part(char const* generator_args)
{
  struct LocalKaGenGraph
  {
    using Part = ExplicitSortedContinuousWorldPart;
    Buffer    fw_buffer;
    Buffer    bw_buffer;
    Part      part;
    index_t   m;
    index_t   local_fw_m;
    index_t   local_bw_m;
    index_t*  fw_head;
    vertex_t* fw_csr;
    index_t*  bw_head;
    vertex_t* bw_csr;
  };

  auto fw_graph = kagen_forward_graph_part(generator_args);
  auto bw_graph = backward_complement_graph_part(fw_graph.part, fw_graph.local_m, fw_graph.head, fw_graph.csr);

  LocalKaGenGraph result;
  result.fw_buffer  = std::move(fw_graph.buffer);
  result.bw_buffer  = std::move(bw_graph.buffer);
  result.part       = fw_graph.part;
  result.m          = fw_graph.m;
  result.local_fw_m = fw_graph.local_m;
  result.local_bw_m = bw_graph.local_m;
  result.fw_head    = fw_graph.head;
  result.bw_head    = bw_graph.head;
  result.fw_csr     = fw_graph.csr;
  result.bw_csr     = bw_graph.csr;
  return result;
}
