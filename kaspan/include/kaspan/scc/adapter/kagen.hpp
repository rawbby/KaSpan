#pragma once

#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/backward_complement.hpp>
#include <kaspan/graph/part.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <kagen.h>

namespace kaspan {

inline auto
kagen_forward_graph_part(
  char const* generator_args)
{
  kagen::KaGen graph_generator{ mpi_basic::comm_world };
  graph_generator.UseCSRRepresentation();

  auto const ka_graph   = graph_generator.GenerateFromOptionString(generator_args);
  auto const global_n   = integral_cast<vertex_t>(ka_graph.NumberOfGlobalVertices());
  auto const global_m   = integral_cast<index_t>(ka_graph.NumberOfGlobalEdges());
  auto const local_n    = integral_cast<vertex_t>(ka_graph.NumberOfLocalVertices());
  auto const local_fw_m = integral_cast<index_t>(ka_graph.NumberOfLocalEdges());
  auto const vertex_end = integral_cast<vertex_t>(ka_graph.vertex_range.second);

  auto  storage = make_buffer<index_t, vertex_t>(local_n + 1, local_fw_m);
  void* memory  = storage.data();

  auto  part_storage = make_buffer<i32>(mpi_basic::world_size);
  void* part_memory  = part_storage.data();
  auto  part         = explicit_sorted_continuous_world_part{ global_n, vertex_end, mpi_basic::world_rank, mpi_basic::world_size, &part_memory };
  DEBUG_ASSERT_EQ(part.n, global_n);
  DEBUG_ASSERT_EQ(part.local_n(), local_n);

  auto const m       = global_m;
  auto const local_m = local_fw_m;
  auto       head    = borrow_array<index_t>(&memory, local_n + 1);
  auto       csr     = borrow_array<vertex_t>(&memory, local_fw_m);

  for (vertex_t k = 0; k <= local_n; ++k) {
    DEBUG_ASSERT_IN_RANGE(ka_graph.xadj[k], 0, local_fw_m + 1, "k={}", k);
    head[k] = integral_cast<index_t>(ka_graph.xadj[k]);
    DEBUG_ASSERT_IN_RANGE(head[k], 0, local_fw_m + 1, "k={}", k);
  }
  for (index_t it = 0; it < local_fw_m; ++it) {
    DEBUG_ASSERT_IN_RANGE(ka_graph.adjncy[it], 0, global_n);
    csr[it] = integral_cast<vertex_t>(ka_graph.adjncy[it]);
    DEBUG_ASSERT_IN_RANGE(csr[it], 0, global_n);
  }

  DEBUG_ASSERT_VALID_GRAPH_PART(part, head, csr);
  return PACK(part_storage, part, storage, m, local_m, head, csr);
}

inline auto
kagen_graph_part(
  char const* generator_args)
{
  auto [part_storage, part, fw_storage, m, local_fw_m, fw_head, fw_csr] = kagen_forward_graph_part(generator_args);

  auto [bw_storage, local_bw_m, bw_head, bw_csr] = backward_complement_graph_part(part, local_fw_m, fw_head, fw_csr);

  return PACK(part_storage, part, m, local_fw_m, local_bw_m, fw_storage, fw_head, fw_csr, bw_storage, bw_head, bw_csr);
}

} // namespace kaspan
