#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/memory/accessor/stack.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/scc/adapter/edgelist.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>

#include <mpi.h>

#include <cstring>

namespace kaspan {

inline void
backward_complement_graph(
  bidi_graph_view g)
{
  g.fw_view().debug_validate();
  g.bw_view().debug_check();

  // === zero bw_head and count indegrees in-place ===
  // after this loop: bw_head[0] == 0 and for every vertex v, bw_head[v + 1] == indegree(v)
  if (g.n > 0) {
    std::memset(g.bw.head, 0, (g.n + 1) * sizeof(index_t));
  }
  index_t it = 0;
  DEBUG_ASSERT(g.n == 0 || g.fw.head[0] == 0);
  for (vertex_t u = 0; u < g.n; ++u) {
    DEBUG_ASSERT_IN_RANGE(g.fw.head[u], 0, g.fw.head[u + 1] + 1);
    auto const end = g.fw.head[u + 1];
    for (; it < end; ++it) {
      DEBUG_ASSERT_IN_RANGE(g.fw.csr[it], 0, g.n);
      ++g.bw.head[g.fw.csr[it] + 1];
    }
  }

  // === convert indegrees to exclusive prefix sums (row starts) in-place ===
  // after the loop: bw_head[u + 1] == sum_{k < u} indegree(k)
  // thus bw_head[v + 1] = row_begin(v) for the transposed graph
  index_t acc = 0;
  for (vertex_t u = 0; u < g.n; ++u) {
    DEBUG_ASSERT_IN_RANGE(g.bw.head[u + 1], 0, g.fw.head[g.n] + 1);
    auto const indegree = g.bw.head[u + 1];
    g.bw.head[u + 1]    = acc;
    acc += indegree;
  }

  // === fill the transposed CSR using bw_head[v + 1] as a per-row write cursor ===
  // for each edge (u,v), write u at bw_csr[bw_head[v + 1]] and increment that cursor
  // upon completion, each cursor advanced by indegree(v), so bw_head[v + 1] == row_end(v)
  it = 0;
  for (vertex_t u = 0; u < g.n; ++u) {
    auto const end = g.fw.head[u + 1];
    for (; it < end; ++it) {
      g.bw.csr[g.bw.head[g.fw.csr[it] + 1]++] = u;
    }
  }

  g.bw_view().debug_validate();
}

template<part_concept Part>
void
backward_complement_graph_part(
  bidi_graph_part<Part>& g)
{
  auto const local_n = g.part.local_n();
  DEBUG_ASSERT_EQ(g.local_fw_m, g.fw.head[local_n]);

  if (mpi_basic::world_size == 1) [[unlikely]] {
    g.local_bw_m = g.local_fw_m;
    g.bw.head    = line_alloc<index_t>(local_n + 1);
    g.bw.csr     = line_alloc<vertex_t>(g.local_bw_m);
    backward_complement_graph(bidi_graph_view{ local_n, g.local_fw_m, g.fw.head, g.fw.csr, g.bw.head, g.bw.csr });
    return;
  }

  auto send_stack                     = make_stack<edge_t>(g.local_fw_m);
  auto [sb, send_counts, send_displs] = mpi_basic::counts_and_displs();
  std::memset(send_counts, 0, mpi_basic::world_size * sizeof(MPI_Count));

  index_t it = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u   = g.part.to_global(k);
    auto const end = g.fw.head[k + 1];
    for (; it < end; ++it) {
      auto const v = g.fw.csr[it];
      send_stack.push({ v, u });
      ++send_counts[g.part.world_rank_of(v)];
    }
  }

  i8 any_edges = g.local_fw_m > 0 ? 1 : 0;
  mpi_basic::allreduce_inplace(&any_edges, 1, mpi_basic::lor);

  if (any_edges == 0) [[unlikely]] {
    g.local_bw_m = 0;
    g.bw.head    = local_n > 0 ? line_alloc<index_t>(local_n + 1) : nullptr;
    g.bw.csr     = nullptr;
    return;
  }

  auto [rb, recv_counts, recv_displs] = mpi_basic::counts_and_displs();
  mpi_basic::alltoallv_counts(send_counts, recv_counts);
  auto const recv_count = mpi_basic::displs(recv_counts, recv_displs);
  mpi_basic::inplace_partition_by_rank(send_stack.data(), send_counts, send_displs, [&](edge_t const& e) {
    return g.part.world_rank_of(e.u);
  });

  auto recv_buffer = make_array<edge_t>(recv_count);

  mpi_basic::alltoallv(send_stack.data(), send_counts, send_displs, recv_buffer.data(), recv_counts, recv_displs, mpi_edge_t);

  g.local_bw_m = recv_count;
  g.bw.csr     = line_alloc<vertex_t>(g.local_bw_m);

  edgelist_to_graph_part(g.part, recv_count, recv_buffer.data(), g.bw.head, g.bw.csr);
}

} // namespace kaspan
