#pragma once

#include <kagen.h>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/backward_complement.hpp>
#include <kaspan/scc/part.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

namespace kaspan {

inline auto
kagen_forward_graph_part(char const* generator_args)
{
  struct
  {
    using part_t = explicit_sorted_continuous_world_part;
    buffer    storage;
    part_t    part;
    index_t   m{};
    index_t   local_m{};
    index_t*  head{};
    vertex_t* csr{};
  } result;

  kagen::KaGen graph_generator{ mpi_basic::comm_world };
  graph_generator.UseCSRRepresentation();

  auto const ka_graph   = graph_generator.GenerateFromOptionString(generator_args);
  auto const global_n   = integral_cast<vertex_t>(ka_graph.NumberOfGlobalVertices());
  auto const global_m   = integral_cast<index_t>(ka_graph.NumberOfGlobalEdges());
  auto const local_n    = integral_cast<vertex_t>(ka_graph.NumberOfLocalVertices());
  auto const local_fw_m = integral_cast<index_t>(ka_graph.NumberOfLocalEdges());
  auto const vertex_end = integral_cast<vertex_t>(ka_graph.vertex_range.second);

  result.storage = make_buffer<i32, index_t, vertex_t>(mpi_basic::world_size, local_n + 1, local_fw_m);
  void* memory   = result.storage.data();

  result.part = explicit_sorted_continuous_world_part{ global_n, vertex_end, mpi_basic::world_rank, mpi_basic::world_size, &memory };
  DEBUG_ASSERT_EQ(result.part.n, global_n);
  DEBUG_ASSERT_EQ(result.part.local_n(), local_n);

  result.m       = global_m;
  result.local_m = local_fw_m;
  result.head    = borrow_array<index_t>(&memory, local_n + 1);
  result.csr     = borrow_array<vertex_t>(&memory, local_fw_m);

  for (vertex_t k = 0; k <= local_n; ++k) {
    DEBUG_ASSERT_IN_RANGE(ka_graph.xadj[k], 0, local_fw_m + 1, "k={}", k);
    result.head[k] = integral_cast<index_t>(ka_graph.xadj[k]);
    DEBUG_ASSERT_IN_RANGE(result.head[k], 0, local_fw_m + 1, "k={}", k);
  }
  for (index_t it = 0; it < local_fw_m; ++it) {
    DEBUG_ASSERT_IN_RANGE(ka_graph.adjncy[it], 0, global_n);
    result.csr[it] = integral_cast<vertex_t>(ka_graph.adjncy[it]);
    DEBUG_ASSERT_IN_RANGE(result.csr[it], 0, global_n);
  }

  DEBUG_ASSERT_VALID_GRAPH_PART(result.part, result.head, result.csr);
  return result;
}

inline auto
kagen_graph_part(char const* generator_args)
{
  struct local_ka_gen_graph
  {
    using part_t = explicit_sorted_continuous_world_part;
    buffer    fw_storage;
    buffer    bw_storage;
    part_t    part;
    index_t   m{};
    index_t   local_fw_m{};
    index_t   local_bw_m{};
    index_t*  fw_head{};
    vertex_t* fw_csr{};
    index_t*  bw_head{};
    vertex_t* bw_csr{};
  };

  auto fw_graph = kagen_forward_graph_part(generator_args);
  auto bw_graph = backward_complement_graph_part(fw_graph.part, fw_graph.local_m, fw_graph.head, fw_graph.csr);

  local_ka_gen_graph result;
  result.fw_storage = std::move(fw_graph.storage);
  result.bw_storage = std::move(bw_graph.storage);
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

} // namespace kaspan
