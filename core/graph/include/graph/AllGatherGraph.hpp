#pragma once

#include <buffer/Buffer.hpp>
#include <comm/SyncFrontierComm.hpp>
#include <graph/Base.hpp>
#include <graph/Graph.hpp>
#include <graph/GraphPart.hpp>

#include <kamping/collectives/allgather.hpp>
#include <kamping/communicator.hpp>

inline void
ComplementBackwardsGraph(Graph& graph)
{
  // === zero bw_head and count indegrees in-place ===
  // after this loop: bw_head[0] == 0 and for every vertex v, bw_head[v + 1] == indegree(v)
  std::memset(graph.bw_head.data(), 0, graph.bw_head.bytes());
  graph.foreach_fw_edge([&graph](vertex_t /* u */, vertex_t v) {
    ++graph.bw_head[v + 1];
  });

  // === convert indegrees to exclusive prefix sums (row starts) in-place ===
  // after the loop: bw_head[u + 1] == sum_{k < u} indegree(k)
  // thus bw_head[v + 1] = row_begin(v) for the transposed graph
  vertex_t prevDegree = 0;
  for (vertex_t u = 0; u < graph.n; ++u) {
    auto const thisDegree = graph.bw_head[u + 1];
    graph.bw_head[u + 1]  = graph.bw_head[u] + prevDegree;
    prevDegree            = thisDegree;
  }

  // === fill the transposed CSR using bw_head[v + 1] as a per-row write cursor ===
  // for each edge (u,v), write u at bw_csr[bw_head[v + 1]] and increment that cursor
  // upon completion, each cursor advanced by indegree(v), so bw_head[v + 1] == row_end(v)
  graph.foreach_fw_edge([&graph](vertex_t u, vertex_t v) {
    graph.bw_csr[graph.bw_head[v + 1]++] = u;
  });
}

template<WorldPartConcept Part>
  requires(not IndexBuffer::compressed)
void
ComplementBackwardsGraph(GraphPart<Part>& graph)
{
  kamping::Communicator<> comms;
  ASSERT_TRY(auto comm, SyncAlltoallvBase<SyncEdgeComm<Part>>::create(comms, SyncEdgeComm{ graph.part }));

  // send alltoall the backward edges they are recived based on v
  graph.foreach_fw_edge([&comm](auto u, auto v) {
    comm.push_relaxed({ v, u });
  });
  comm.communicate(comms);
  std::ranges::sort(comm.recv_buf().begin(), comm.recv_buf().end(), EdgeLess{});

  ASSERT_TRY(graph.bw_head, IndexBuffer::create(graph.fw_head.size()));
  ASSERT_TRY(graph.bw_csr, VertexBuffer::create(comm.recv_buf().size()));

  // === zero bw_head and count indegrees in-place ===
  // after this loop: bw_head[0] == 0 and for every vertex v, bw_head[v + 1] == indegree(v)
  if (graph.bw_head.size()) {
    memset(graph.bw_head.data(), 0, graph.bw_head.bytes());
    for (auto const [v, u] : comm.recv_buf()) {
      auto const k = graph.part.rank(v);
      ++graph.bw_head[k + 1];
    }
  }

  // === convert indegrees to exclusive prefix sums (row starts) in-place ===
  // after the loop: bw_head[u + 1] == sum_{k < u} indegree(k)
  // thus bw_head[v + 1] = row_begin(v) for the transposed graph
  vertex_t prevDegree = 0;
  for (vertex_t k = 0; k < graph.part.size(); ++k) {
    auto const thisDegree = graph.bw_head[k + 1];
    graph.bw_head[k + 1]  = graph.bw_head[k] + prevDegree;
    prevDegree            = thisDegree;
  }

  // === fill the transposed CSR using bw_head[v + 1] as a per-row write cursor ===
  // for each edge (u,v), write u at bw_csr[bw_head[v + 1]] and increment that cursor
  // upon completion, each cursor advanced by indegree(v), so bw_head[v + 1] == row_end(v)
  for (auto const [v, u] : comm.recv_buf()) {
    auto const k                         = graph.part.rank(v);
    graph.bw_csr[graph.bw_head[k + 1]++] = u;
  }
}

/* gathers a `GraphPart` on every node */
template<WorldPartConcept Part>
  requires(not Part::ordered and not IndexBuffer::compressed)
auto
AllGatherGraph(kamping::Communicator<>& comm, GraphPart<Part> const& graph_part) -> Result<Graph>
{
  Graph result_graph;
  result_graph.n = graph_part.n;
  result_graph.m = graph_part.m;
  RESULT_TRY(result_graph.fw_head, create_head_buffer(graph_part.n, graph_part.m));
  RESULT_TRY(result_graph.bw_head, create_head_buffer(graph_part.n, graph_part.m));
  RESULT_TRY(result_graph.bw_csr, create_csr_buffer(graph_part.n, graph_part.m));
  RESULT_TRY(result_graph.fw_csr, create_csr_buffer(graph_part.n, graph_part.m));

  // allocate and setup forward head by:
  // 1. fill result zero
  // 2. insert local degree into global head slots (row_end)
  // 3. synchronize via all reduce max inplace
  // 4. calculate the actual head from given counts

  std::memset(result_graph.fw_head.data(), 0, result_graph.fw_head.bytes());

  for (size_t k = 0; k < graph_part.part.size(); ++k)
    result_graph.fw_head.set(1 + graph_part.part.select(k), graph_part.fw_degree(k));

  comm.allreduce_inplace(
    kamping::send_recv_buf(std::span{
      static_cast<index_t*>(result_graph.fw_head.data()),
      result_graph.n + 1 }),
    kamping::op(kamping::ops::max{}));

  for (size_t i = 0; i < graph_part.n; ++i) {
    auto const pre = result_graph.fw_head.get(i);
    auto const cur = result_graph.fw_head.get(1 + i);
    result_graph.fw_head.set(1 + i, pre + cur);
  }

  // allocate and setup forward csr by:
  // 1. fill result zero
  // 2. insert local data into global slots
  // 3. synchronize via all reduce max inplace

  std::memset(result_graph.fw_csr.data(), 0, result_graph.fw_csr.bytes());

  for (size_t k = 0; k < graph_part.part.size(); ++k) {
    auto const begin = graph_part.fw_head.get(k);
    auto const end   = graph_part.fw_head.get(k + 1);
    auto const u     = graph_part.part.select(k);
    auto const head  = result_graph.fw_head[u];
    for (size_t it = begin; it < end; ++it) {
      auto const v                             = graph_part.fw_csr[it];
      result_graph.fw_csr[head + (it - begin)] = v;
    }
  }

  comm.allreduce_inplace(
    kamping::send_recv_buf(std::span{
      static_cast<vertex_t*>(result_graph.fw_csr.data()),
      graph_part.m }),
    kamping::op(kamping::ops::max{}));

  // reconstruct backward head and csr from forward
  ComplementBackwardsGraph(result_graph);

  return result_graph;
}

/* gathers a `GraphPart` on every node */
template<WorldPartConcept Part>
  requires(Part::ordered and not IndexBuffer::compressed)
auto
AllGatherGraph(kamping::Communicator<>& comm, GraphPart<Part> const& graph_part) -> Result<Graph>
{
  Graph result_graph;
  result_graph.n = graph_part.n;
  result_graph.m = graph_part.m;
  RESULT_TRY(result_graph.fw_head, create_head_buffer(graph_part.n, graph_part.m));
  RESULT_TRY(result_graph.fw_csr, create_csr_buffer(graph_part.n, graph_part.m));
  RESULT_TRY(result_graph.bw_head, create_head_buffer(graph_part.n, graph_part.m));
  RESULT_TRY(result_graph.bw_csr, create_csr_buffer(graph_part.n, graph_part.m));

  // directly sync all graph partition heads and create global head later
  comm.allgatherv(
    kamping::send_buf(std::span{
      static_cast<index_t const*>(graph_part.fw_head.data()) + 1,
      graph_part.part.size() }),
    kamping::recv_buf(std::span{
      static_cast<index_t*>(result_graph.fw_head.data()) + 1,
      result_graph.n }));

  // rank by rank fix the head by applying an offset based on prior heads
  result_graph.fw_head.set(0, 0);
  size_t i = 1;
  for (size_t rank = 0; rank < comm.size(); ++rank) {
    auto const rank_part   = graph_part.part.world_part_of(rank);
    auto const rank_offset = result_graph.fw_head.get(i - 1);
    for (size_t k = 0; k < rank_part.size(); ++k, ++i) {
      result_graph.fw_head.set(i, rank_offset + result_graph.fw_head.get(i));
    }
  }

  // simply sync all edges
  comm.allgatherv(
    kamping::send_buf(std::span{
      static_cast<vertex_t const*>(graph_part.fw_csr.data()),
      graph_part.fw_csr.size() }),
    kamping::recv_buf(std::span{
      static_cast<vertex_t*>(result_graph.fw_csr.data()),
      result_graph.m }));

  // reconstruct backward head and csr from forward
  ComplementBackwardsGraph(result_graph);

  return result_graph;
}

/* gathers an induced subgraph (by vertex filter) on every node; also returns new->old vertex mapping */
template<class Part, class Fn>
  requires(Part::ordered and not IndexBuffer::compressed and std::convertible_to<std::invoke_result_t<Fn, vertex_t>, bool>)
auto
AllGatherSubGraph(GraphPart<Part> const& graph_part, Fn&& in_sub_graph) -> Result<std::tuple<Graph, VertexBuffer, std::unordered_map<vertex_t, vertex_t>>>
{
  kamping::Communicator<> comm;

  std::vector<vertex_t> local_sub_ids_inverse;
  local_sub_ids_inverse.reserve(graph_part.part.size());
  for (size_t k = 0; k < graph_part.part.size(); ++k) {
    if (static_cast<bool>(in_sub_graph(k)))
      local_sub_ids_inverse.push_back(graph_part.part.select(k));
  }

  vertex_t sub_n = local_sub_ids_inverse.size();
  comm.allreduce_inplace(kamping::send_recv_buf(sub_n), kamping::op(kamping::ops::plus{}));

  RESULT_TRY(auto sub_ids_inverse, VertexBuffer::create(sub_n));
  comm.allgatherv(
    kamping::send_buf(std::span{
      local_sub_ids_inverse.data(),
      local_sub_ids_inverse.size() }),
    kamping::recv_buf(std::span{
      static_cast<vertex_t*>(sub_ids_inverse.data()),
      sub_ids_inverse.size() }));

  std::unordered_map<vertex_t, vertex_t> sub_ids;
  sub_ids.reserve(sub_ids_inverse.size());
  for (vertex_t new_id = 0; new_id < sub_ids_inverse.size(); ++new_id) {
    sub_ids.emplace(sub_ids_inverse[new_id], new_id);
  }

  std::vector<index_t>  local_sub_degrees;
  std::vector<vertex_t> local_sub_csr;
  local_sub_degrees.reserve(local_sub_ids_inverse.size());

  for (vertex_t const u : local_sub_ids_inverse) {
    auto const k   = graph_part.part.rank(u);
    auto const beg = graph_part.fw_head.get(k);
    auto const end = graph_part.fw_head.get(k + 1);

    index_t deg = 0;
    for (index_t it = beg; it < end; ++it) {
      auto const v = graph_part.fw_csr[it];
      if (static_cast<bool>(in_sub_graph(v))) {
        ASSERT(sub_ids.contains(v));
        local_sub_csr.push_back(sub_ids.find(v)->second);
        ++deg;
      }
    }
    local_sub_degrees.push_back(deg);
  }

  Graph sub_graph;

  // gather the head by first storing degrees and than create head with prefix sum
  sub_graph.n = sub_n;
  RESULT_TRY(sub_graph.fw_head, IndexBuffer::create(sub_n + 1));
  comm.allgatherv(
    kamping::send_buf(std::span{
      local_sub_degrees.data(),
      local_sub_degrees.size() }),
    kamping::recv_buf(std::span{
      static_cast<index_t*>(sub_graph.fw_head.data()) + 1,
      sub_n }));
  sub_graph.fw_head.set(0, 0);
  for (size_t i = 0; i < sub_n + 1; ++i) {
    sub_graph.fw_head.set(i + 1, sub_graph.fw_head.get(i) + sub_graph.fw_head.get(i + 1));
  }

  // gather the csr
  sub_graph.m = sub_graph.fw_head.get(sub_n);
  RESULT_TRY(sub_graph.fw_csr, VertexBuffer::create(sub_graph.m));
  comm.allgatherv(
    kamping::send_buf(std::span{
      local_sub_csr.data(),
      local_sub_csr.size() }),
    kamping::recv_buf(std::span{
      static_cast<vertex_t*>(sub_graph.fw_csr.data()),
      sub_graph.m }));

  // reconstruct backward head and csr from forward
  RESULT_TRY(sub_graph.bw_head, IndexBuffer::create(sub_n + 1));
  RESULT_TRY(sub_graph.bw_csr, VertexBuffer::create(sub_graph.m));
  ComplementBackwardsGraph(sub_graph);

  return std::tuple{ std::move(sub_graph), std::move(sub_ids_inverse), std::move(sub_ids) };
}
