#pragma once

#include <kagen.h>
#include <memory/buffer.hpp>
#include <scc/backward_complement.hpp>
#include <scc/part.hpp>
#include <util/result.hpp>

inline auto
kagen_forward_graph_part(char const* generator_args)
{
  struct LocalKaGenForwardGraph
  {
    using Part = ExplicitSortedContinuousWorldPart;
    Buffer    buffer;
    Part      part;
    index_t   m;
    index_t   local_m;
    index_t*  head;
    vertex_t* csr;
  };

  kagen::KaGen graph_generator{ MPI_COMM_WORLD };
  graph_generator.UseCSRRepresentation();

  auto const     ka_graph   = graph_generator.GenerateFromOptionString(generator_args);
  vertex_t const global_n   = ka_graph.NumberOfGlobalVertices();
  index_t const  global_m   = ka_graph.NumberOfGlobalEdges();
  vertex_t const local_n    = ka_graph.NumberOfLocalVertices();
  index_t const  local_m    = ka_graph.NumberOfLocalEdges();
  vertex_t const vertex_end = ka_graph.vertex_range.second;

  LocalKaGenForwardGraph result;

  auto const part_bytes = page_ceil<i32>(mpi_world_size);
  auto const head_bytes = page_ceil<index_t>(local_n + 1);
  auto const csr_bytes  = page_ceil<vertex_t>(local_m);

  result.buffer = Buffer::create(part_bytes + head_bytes + csr_bytes);
  void* memory  = result.buffer.data();

  result.part    = ExplicitSortedContinuousWorldPart{ global_n, vertex_end, mpi_world_rank, mpi_world_size, memory };
  result.m       = global_m;
  result.local_m = local_m;
  result.head    = borrow<index_t>(memory, local_n + 1);
  result.csr     = borrow<vertex_t>(memory, local_m);

  for (vertex_t k = 0; k <= local_n; ++k) {
    result.head[k] = ka_graph.xadj[k];
  }
  for (index_t it = 0; it < local_m; ++it) {
    result.csr[it] = ka_graph.adjncy[it];
  }

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
  auto bw_graph = backward_complement_graph_part(fw_graph.part, fw_graph.m, fw_graph.head, fw_graph.csr);

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
