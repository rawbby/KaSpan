#pragma once

#include "adapter/edgelist.hpp"
#include "graph.hpp"

#include <debug/valgrind.hpp>
#include <memory/accessor/stack_accessor.hpp>
#include <memory/borrow.hpp>
#include <memory/buffer.hpp>
#include <scc/base.hpp>
#include <scc/part.hpp>

#include <cstring>

/**
 * Inplace complement bw_head + bw_csr from fw_head + fw_csr for n vertices.
 * Runs in linear time (O(n+2m)) and zero extra memory.
 *
 * @param[in] n number of vertices in graph
 * @param[in] fw_head valid head array of graph
 * @param[in] fw_csr valid csr array of graph
 * @param[out] bw_head (maybe uninitialized) pre-allocated head array of size n + 1
 * @param[out] bw_csr (maybe uninitialized) pre-allocated csr array of size m
 */
inline void
backward_complement_graph(
  vertex_t        n,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t*        bw_head,
  vertex_t*       bw_csr)
{
  DEBUG_ASSERT_VALID_GRAPH(n, fw_head[n], fw_head, fw_csr);

  // === zero bw_head and count indegrees in-place ===
  // after this loop: bw_head[0] == 0 and for every vertex v, bw_head[v + 1] == indegree(v)
  std::memset(bw_head, 0, (n + 1) * sizeof(index_t));
  index_t it = 0;
  DEBUG_ASSERT(n == 0 or fw_head[0] == 0);
  for (vertex_t u = 0; u < n; ++u) {
    DEBUG_ASSERT_IN_RANGE(fw_head[u], 0, fw_head[u + 1] + 1);
    auto const end = fw_head[u + 1];
    for (; it < end; ++it) {
      DEBUG_ASSERT_IN_RANGE(fw_csr[it], 0, n);
      ++bw_head[fw_csr[it] + 1];
    }
  }

  // === convert indegrees to exclusive prefix sums (row starts) in-place ===
  // after the loop: bw_head[u + 1] == sum_{k < u} indegree(k)
  // thus bw_head[v + 1] = row_begin(v) for the transposed graph
  index_t acc = 0;
  for (vertex_t u = 0; u < n; ++u) {
    DEBUG_ASSERT_IN_RANGE(bw_head[u + 1], 0, fw_head[n] + 1);
    auto const indegree = bw_head[u + 1];
    bw_head[u + 1]      = acc;
    acc += indegree;
  }

  // === fill the transposed CSR using bw_head[v + 1] as a per-row write cursor ===
  // for each edge (u,v), write u at bw_csr[bw_head[v + 1]] and increment that cursor
  // upon completion, each cursor advanced by indegree(v), so bw_head[v + 1] == row_end(v)
  it = 0;
  for (vertex_t u = 0; u < n; ++u) {
    auto const end = fw_head[u + 1];
    for (; it < end; ++it) {
      bw_csr[bw_head[fw_csr[it] + 1]++] = u;
    }
  }

  DEBUG_ASSERT_VALID_GRAPH(n, bw_head[n], bw_head, bw_csr);
}

/**
 * Inplace complement bw_head + bw_csr from fw_head + fw_csr for n vertices.
 * Runs in linear time (O(n+2m)) and zero extra memory.
 *
 * @param[in] part the partition of the graph stored
 * @param[in] local_m the number of forward edges of graph partition (= head[part.local_n()])
 * @param[in] head valid forward head array of graph partition
 * @param[in] csr valid forward csr array of graph partition
 */
template<WorldPartConcept Part>
auto
backward_complement_graph_part(
  Part const&     part,
  index_t         local_m,
  index_t const*  head,
  vertex_t const* csr)
{
  struct
  {
    Buffer    buffer;
    index_t   local_m = 0;
    index_t*  head    = nullptr;
    vertex_t* csr     = nullptr;
  } result;

  auto const local_n = part.local_n();
  DEBUG_ASSERT_EQ(local_m, head[local_n]);

  if (mpi_basic::world_size == 1) [[unlikely]] {
    result.local_m = local_m;
    result.buffer  = make_fw_graph_buffer(local_n, local_m);
    auto* memory   = result.buffer.data();
    result.head    = borrow_array<index_t>(&memory, local_n + 1);
    result.csr     = borrow_array<vertex_t>(&memory, local_m);
    backward_complement_graph(local_n, head, csr, result.head, result.csr);
    return result;
  }

  auto  send_stack_buffer = make_buffer<Edge>(local_m);
  void* send_stack_memory = send_stack_buffer.data();

  StackAccessor<Edge> send_stack{ send_stack_memory };
  auto [sb, send_counts, send_displs] = mpi_basic::counts_and_displs();
  std::memset(send_counts, 0, mpi_basic::world_size * sizeof(MPI_Count));

  index_t it = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u   = part.to_global(k);
    auto const end = head[k + 1];
    for (; it < end; ++it) {
      auto const v = csr[it];
      send_stack.push({ v, u });
      ++send_counts[part.world_rank_of(v)];
    }
  }

  i8 any_edges = local_m > 0 ? 1 : 0;
  mpi_basic::allreduce_inplace(&any_edges, 1, mpi_basic::lor);

  if (any_edges == 0) [[unlikely]] {
    result.local_m = 0;
    result.buffer  = make_graph_buffer(local_n, 0);
    auto* memory   = result.buffer.data();
    result.head    = borrow_array_clean<index_t>(&memory, local_n + 1);
    result.csr     = nullptr;
    return result;
  }

  auto [rb, recv_counts, recv_displs] = mpi_basic::counts_and_displs();
  mpi_basic::alltoallv_counts(send_counts, recv_counts);
  auto const recv_count = mpi_basic::displs(recv_counts, recv_displs);
  mpi_basic::inplace_partition_by_rank(send_stack.data(), send_counts, send_displs, [&part](Edge const& e) {
    return part.world_rank_of(e.u);
  });

  auto  recv_buffer = make_buffer<Edge>(recv_count);
  auto* recv_access = static_cast<Edge*>(recv_buffer.data());
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(recv_access, recv_count * sizeof(Edge));
  mpi_basic::alltoallv(send_stack.data(), send_counts, send_displs, recv_access, recv_counts, recv_displs, mpi_edge_t);

  result.local_m = recv_count;
  result.buffer  = make_fw_graph_buffer(local_n, result.local_m);
  auto* mem      = result.buffer.data();
  result.head    = borrow_array<index_t>(&mem, local_n + 1);
  result.csr     = borrow_array<vertex_t>(&mem, result.local_m);

  edgelist_to_graph_part(part, recv_count, recv_access, result.head, result.csr);
  return result;
}
