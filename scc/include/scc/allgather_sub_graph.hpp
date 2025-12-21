#pragma once

#include <memory/accessor/stack_accessor.hpp>
#include <memory/buffer.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <scc/part.hpp>
#include <util/return_pack.hpp>

#include <unordered_map>

namespace sub_graph {

template<class Part, class Fn>
auto
allgather_sub_ids(Part const& part, vertex_t local_sub_n, Fn&& in_sub_graph)
{
  auto const local_n = part.local_n();

  auto [TMP(), local_ids_inverse_stack] = StackAccessor<vertex_t>::create(local_sub_n);
  for (vertex_t k = 0; k < local_n; ++k) {
    if (in_sub_graph(k)) {
      local_ids_inverse_stack.push(part.to_global(k));
    }
  }
  DEBUG_ASSERT_EQ(local_ids_inverse_stack.size(), local_sub_n);

  auto [TMP(), counts, displs] = mpi_basic_counts_and_displs();

  mpi_basic_allgatherv_counts(local_ids_inverse_stack.size(), counts);
  auto const sub_n = static_cast<vertex_t>(mpi_basic_displs(counts, displs));

  auto ids_inverse_buffer = make_buffer<vertex_t>(sub_n);
  auto ids_inverse        = static_cast<vertex_t*>(ids_inverse_buffer.data());
  mpi_basic_allgatherv<vertex_t>(local_ids_inverse_stack.data(), local_ids_inverse_stack.size(), ids_inverse, counts, displs);

  auto* local_ids_inverse = ids_inverse + displs[mpi_world_rank];
  return PACK(
    sub_n,
    ids_inverse_buffer,
    local_ids_inverse,
    ids_inverse);
}

template<class Part>
auto
allgather_csr_degrees(
  Part const&     part,
  vertex_t        local_sub_n,
  index_t const*  head,
  vertex_t const* csr,

  vertex_t const*                               local_ids_inverse,
  std::unordered_map<vertex_t, vertex_t> const& sub_ids)
{
  auto const local_n = part.local_n();
  auto const local_m = head[local_n];

  auto buffer = make_buffer<MPI_Count, MPI_Aint, vertex_t, index_t>(
    mpi_world_size, mpi_world_size, local_m, mpi_world_root + local_sub_n);
  void* memory = buffer.data();

  auto [counts, displs]  = mpi_basic_counts_and_displs(memory);
  auto local_sub_csr     = StackAccessor<vertex_t>::borrow(memory, local_m);
  auto local_sub_degrees = StackAccessor<index_t>::borrow(memory, local_sub_n + mpi_world_root);
  if (mpi_world_root) {
    local_sub_degrees.push(0);
  }

  for (vertex_t i = 0; i < local_sub_n; ++i) { // for each vertex in part and in sub graph
    auto const u   = local_ids_inverse[i];
    auto const k   = part.to_local(u);
    auto const beg = head[k];
    auto const end = head[k + 1];

    index_t deg = 0;
    for (index_t it = beg; it < end; ++it) {
      auto const v = csr[it];
      // v is in sub graph if contained by sub_ids
      if (auto const jt = sub_ids.find(v); jt != sub_ids.end()) {
        local_sub_csr.push(jt->second);
        ++deg;
      }
    }
    local_sub_degrees.push(deg);
  }

  mpi_basic_allgatherv_counts(local_sub_csr.size(), counts);
  auto const sub_m = mpi_basic_displs(counts, displs);

  return PACK(
    buffer,
    sub_m,
    counts,
    displs,
    local_sub_csr,
    local_sub_degrees);
}

}

template<class Part, class Fn>
  requires(Part::ordered and std::convertible_to<std::invoke_result_t<Fn, vertex_t>, bool>)
auto
allgather_sub_graph(
  Part const&     part,
  vertex_t        local_sub_n,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  Fn&&            in_sub_graph)
{
  struct // NOLINT(*-pro-type-member-init)
  {
    Buffer    ids_inverse_buffer;
    vertex_t* ids_inverse;

    Buffer    buffer;
    vertex_t  n;
    index_t   m;
    index_t*  fw_head;
    vertex_t* fw_csr;
    index_t*  bw_head;
    vertex_t* bw_csr;
  } sub;

  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(part, bw_head, bw_csr);

  auto [sub_n, ids_inverse_buffer, local_ids_inverse, ids_inverse] =
    sub_graph::allgather_sub_ids(part, local_sub_n, in_sub_graph);

  sub.ids_inverse_buffer = std::move(ids_inverse_buffer);
  sub.ids_inverse        = ids_inverse;
  sub.n                  = sub_n;

  std::unordered_map<vertex_t, vertex_t> sub_ids;
  sub_ids.reserve(sub_n);
  for (vertex_t new_id = 0; new_id < sub_n; ++new_id) {
    sub_ids.emplace(ids_inverse[new_id], new_id);
  }

  // communicate forward graph
  {
    auto [TMP(), sub_m, counts, displs, local_sub_fw_csr, local_sub_fw_degrees] =
      sub_graph::allgather_csr_degrees(part, local_sub_n, fw_head, fw_csr, local_ids_inverse, sub_ids);

    sub.m        = sub_m;
    sub.buffer   = make_graph_buffer(sub_n, sub_m);
    void* memory = sub.buffer.data();
    sub.fw_head  = borrow<index_t>(memory, sub_n + 1);
    sub.fw_csr   = borrow<vertex_t>(memory, sub_m);
    sub.bw_head  = borrow<index_t>(memory, sub_n + 1);
    sub.bw_csr   = borrow<vertex_t>(memory, sub_m);

    mpi_basic_allgatherv<vertex_t>(local_sub_fw_csr.data(), local_sub_fw_csr.size(), sub.fw_csr, counts, displs);
    auto const send_count = local_sub_fw_degrees.size();
    mpi_basic_allgatherv_counts(send_count, counts);
    auto const recv_count = mpi_basic_displs(counts, displs);
    DEBUG_ASSERT_EQ(sub.n + 1, recv_count);
    mpi_basic_allgatherv<index_t>(local_sub_fw_degrees.data(), send_count, sub.fw_head, counts, displs);
    for (vertex_t u = 0; u < sub_n; ++u) {
      sub.fw_head[u + 1] = sub.fw_head[u] + sub.fw_head[u + 1];
    }
  }

  // communicate backward graph
  {
    auto [TMP(), sub_m, counts, displs, local_sub_bw_csr, local_sub_bw_degrees] =
      sub_graph::allgather_csr_degrees(part, local_sub_n, bw_head, bw_csr, local_ids_inverse, sub_ids);

    DEBUG_ASSERT_EQ(sub.m, sub_m);

    mpi_basic_allgatherv<vertex_t>(local_sub_bw_csr.data(), local_sub_bw_csr.size(), sub.bw_csr, counts, displs);
    auto const send_count = local_sub_bw_degrees.size();
    mpi_basic_allgatherv_counts(send_count, counts);
    auto const recv_count = mpi_basic_displs(counts, displs);
    DEBUG_ASSERT_EQ(sub.n + 1, recv_count);
    mpi_basic_allgatherv<index_t>(local_sub_bw_degrees.data(), send_count, sub.bw_head, counts, displs);
    for (vertex_t u = 0; u < sub_n; ++u) {
      sub.bw_head[u + 1] = sub.bw_head[u] + sub.bw_head[u + 1];
    }
  }

  DEBUG_ASSERT_VALID_GRAPH(sub.n, sub.fw_head[sub.n], sub.fw_head, sub.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(sub.n, sub.bw_head[sub.n], sub.bw_head, sub.bw_csr);
  return sub;
}

template<class Part, class Fn>
  requires(Part::ordered and std::convertible_to<std::invoke_result_t<Fn, vertex_t>, bool>)
auto
allgather_fw_sub_graph(
  Part const&     part,
  vertex_t        local_sub_n,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  Fn&&            in_sub_graph)
{
  struct // NOLINT(*-pro-type-member-init)
  {
    Buffer    ids_inverse_buffer;
    vertex_t* ids_inverse;

    Buffer    buffer;
    vertex_t  n;
    index_t   m;
    index_t*  fw_head;
    vertex_t* fw_csr;
  } sub;

  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);

  auto [sub_n, ids_inverse_buffer, local_ids_inverse, ids_inverse] =
    sub_graph::allgather_sub_ids(part, local_sub_n, in_sub_graph);

  sub.ids_inverse_buffer = std::move(ids_inverse_buffer);
  sub.ids_inverse        = ids_inverse;
  sub.n                  = sub_n;

  std::unordered_map<vertex_t, vertex_t> sub_ids;
  sub_ids.reserve(sub_n);
  for (vertex_t new_id = 0; new_id < sub_n; ++new_id) {
    sub_ids.emplace(ids_inverse[new_id], new_id);
  }

  // communicate forward graph
  {
    auto [TMP(), sub_m, counts, displs, local_sub_fw_csr, local_sub_fw_degrees] =
      sub_graph::allgather_csr_degrees(part, local_sub_n, fw_head, fw_csr, local_ids_inverse, sub_ids);

    sub.m        = sub_m;
    sub.buffer   = make_fw_graph_buffer(sub_n, sub_m);
    void* memory = sub.buffer.data();
    sub.fw_head  = borrow<index_t>(memory, sub_n + 1);
    sub.fw_csr   = borrow<vertex_t>(memory, sub_m);

    mpi_basic_allgatherv<vertex_t>(local_sub_fw_csr.data(), local_sub_fw_csr.size(), sub.fw_csr, counts, displs);
    auto const send_count = local_sub_fw_degrees.size();
    mpi_basic_allgatherv_counts(send_count, counts);
    auto const recv_count = mpi_basic_displs(counts, displs);
    DEBUG_ASSERT_EQ(sub.n + 1, recv_count);
    mpi_basic_allgatherv<index_t>(local_sub_fw_degrees.data(), send_count, sub.fw_head, counts, displs);
    for (vertex_t u = 0; u < sub_n; ++u) {
      sub.fw_head[u + 1] = sub.fw_head[u] + sub.fw_head[u + 1];
    }
  }

  DEBUG_ASSERT_VALID_GRAPH(sub.n, sub.fw_head[sub.n], sub.fw_head, sub.fw_csr);
  return sub;
}
