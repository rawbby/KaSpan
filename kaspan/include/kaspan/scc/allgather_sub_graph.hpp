#pragma once

#include "kaspan/graph/bidi_graph.hpp"

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/part.hpp>
#include <kaspan/memory/accessor/stack.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <mpi.h>

#include <algorithm>
#include <concepts>
#include <iterator>
#include <type_traits>

namespace kaspan {
namespace sub_graph {

template<world_part_concept part_t,
         class fn_t>
auto
allgather_sub_ids(
  part_t const& part,
  vertex_t      local_sub_n,
  fn_t&&        in_sub_graph)
{
  auto const local_n = part.local_n();

  auto local_super_ids_ = make_stack<vertex_t>(local_sub_n);
  for (vertex_t k = 0; k < local_n; ++k) {
    if (in_sub_graph(k)) {
      local_super_ids_.push(part.to_global(k));
    }
  }
  DEBUG_ASSERT_EQ(local_super_ids_.size(), local_sub_n);

  auto [TMP(), counts, displs] = mpi_basic::counts_and_displs();

  mpi_basic::allgather_counts(local_super_ids_.size(), counts);
  auto const sub_n = integral_cast<vertex_t>(mpi_basic::displs(counts, displs));

  auto super_ids = make_array<vertex_t>(sub_n);
  mpi_basic::allgatherv<vertex_t>(local_super_ids_.data(), local_super_ids_.size(), super_ids.data(), counts, displs);

  auto* local_super_ids = super_ids.data() + displs[mpi_basic::world_rank];
  return PACK(sub_n, super_ids, local_super_ids);
}

template<class part_t>
auto
allgather_csr_degrees(
  part_t const&   part,
  vertex_t        local_sub_n,
  index_t const*  head,
  vertex_t const* csr,

  vertex_t const* local_ids_inverse,
  vertex_t        sub_n,
  vertex_t const* ids_inverse)
{
  auto const local_n = part.local_n();

  index_t local_sub_m_upper_bound = 0;
  for (vertex_t i = 0; i < local_sub_n; ++i) {
    auto const u = local_ids_inverse[i];
    auto const k = part.to_local(u);
    local_sub_m_upper_bound += head[k + 1] - head[k];
  }

  auto  buffer = make_buffer<MPI_Count, MPI_Aint, vertex_t, index_t>(mpi_basic::world_size, mpi_basic::world_size, local_sub_m_upper_bound, mpi_basic::world_root + local_sub_n);
  void* memory = buffer.data();

  auto [counts, displs]  = mpi_basic::counts_and_displs(&memory);
  auto local_sub_csr     = borrow_stack<vertex_t>(&memory, local_sub_m_upper_bound);
  auto local_sub_degrees = borrow_stack<index_t>(&memory, local_sub_n + mpi_basic::world_root);
  if (mpi_basic::world_root) {
    local_sub_degrees.push(0);
  }

  for (vertex_t i = 0; i < local_sub_n; ++i) { // for each vertex in part and in sub graph
    auto const u   = local_ids_inverse[i];
    auto const k   = part.to_local(u);
    auto const beg = head[k];
    auto const end = head[k + 1];

    index_t deg = 0;
    for (index_t it = beg; it < end; ++it) {
      auto const        v  = csr[it];
      auto const* const jt = std::lower_bound(ids_inverse, ids_inverse + sub_n, v);
      if (jt != ids_inverse + sub_n and *jt == v) {
        local_sub_csr.push(integral_cast<vertex_t>(std::distance(ids_inverse, jt)));
        ++deg;
      }
    }
    local_sub_degrees.push(deg);
  }

  mpi_basic::allgather_counts(local_sub_csr.size(), counts);
  auto const sub_m = integral_cast<index_t>(mpi_basic::displs(counts, displs));

  return PACK(buffer, sub_m, counts, displs, local_sub_csr, local_sub_degrees);
}

}

template<class part_t,
         class fn_t>
  requires(part_t::ordered and std::convertible_to<std::invoke_result_t<fn_t,
                                                                        vertex_t>,
                                                   bool>)
auto
allgather_sub_graph(
  part_t const&   part,
  vertex_t        local_sub_n,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  fn_t&&          in_sub_graph)
{
  auto [sub_n, ids_inverse, local_ids_inverse] = sub_graph::allgather_sub_ids(part, local_sub_n, in_sub_graph);

  auto bg = bidi_graph{};

  // communicate forward graph
  {
    auto [TMP(), sub_m, counts, displs, local_sub_fw_csr, local_sub_fw_degrees] =
      sub_graph::allgather_csr_degrees(part, local_sub_n, fw_head, fw_csr, local_ids_inverse, sub_n, ids_inverse.data());

    DEBUG_ASSERT_GE(sub_m, 0);
    bg = bidi_graph{ sub_n, sub_m };

    mpi_basic::allgatherv<vertex_t>(local_sub_fw_csr.data(), local_sub_fw_csr.size(), bg.fw.csr, counts, displs);
    auto const send_count = local_sub_fw_degrees.size();
    mpi_basic::allgather_counts(send_count, counts);
    auto const recv_count = mpi_basic::displs(counts, displs);
    DEBUG_ASSERT_EQ(bg.n + 1, recv_count);
    mpi_basic::allgatherv<index_t>(local_sub_fw_degrees.data(), send_count, bg.fw.head, counts, displs);
    for (vertex_t u = 0; u < sub_n; ++u) {
      bg.fw.head[u + 1] = bg.fw.head[u] + bg.fw.head[u + 1];
    }
  }

  // communicate backward graph
  {
    auto [TMP(), sub_m, counts, displs, local_sub_bw_csr, local_sub_bw_degrees] =
      sub_graph::allgather_csr_degrees(part, local_sub_n, bw_head, bw_csr, local_ids_inverse, sub_n, ids_inverse.data());

    DEBUG_ASSERT_EQ(bg.m, sub_m);

    mpi_basic::allgatherv<vertex_t>(local_sub_bw_csr.data(), local_sub_bw_csr.size(), bg.bw.csr, counts, displs);
    auto const send_count = local_sub_bw_degrees.size();
    mpi_basic::allgather_counts(send_count, counts);
    auto const recv_count = mpi_basic::displs(counts, displs);
    DEBUG_ASSERT_EQ(bg.n + 1, recv_count);
    mpi_basic::allgatherv<index_t>(local_sub_bw_degrees.data(), send_count, bg.bw.head, counts, displs);
    for (vertex_t u = 0; u < sub_n; ++u) {
      bg.bw.head[u + 1] = bg.bw.head[u] + bg.bw.head[u + 1];
    }
  }

  return PACK(ids_inverse, bg);
}

template<class part_t,
         class fn_t>
  requires(part_t::ordered and std::convertible_to<std::invoke_result_t<fn_t,
                                                                        vertex_t>,
                                                   bool>)
auto
allgather_fw_sub_graph(
  part_t const&   part,
  vertex_t        local_sub_n,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  fn_t&&          in_sub_graph)
{
  auto [sub_n, super_ids, local_super_ids] = sub_graph::allgather_sub_ids(part, local_sub_n, in_sub_graph);
  DEBUG_ASSERT_EQ(sub_n, mpi_basic::allreduce_single(sub_n, mpi_basic::max));

  auto g = graph{};

  if (sub_n > 0) {

    // communicate forward graph
    {
      auto [TMP(), sub_m, counts, displs, local_sub_fw_csr, local_sub_fw_degrees] =
        sub_graph::allgather_csr_degrees(part, local_sub_n, fw_head, fw_csr, local_super_ids, sub_n, super_ids.data());

      g = graph{ sub_n, sub_m };

      mpi_basic::allgatherv<vertex_t>(local_sub_fw_csr.data(), local_sub_fw_csr.size(), g.csr, counts, displs);
      auto const send_count = local_sub_fw_degrees.size();
      mpi_basic::allgather_counts(send_count, counts);
      auto const recv_count = mpi_basic::displs(counts, displs);
      DEBUG_ASSERT(recv_count == 0 || g.head != nullptr);
      DEBUG_ASSERT_EQ(g.n + 1, recv_count);
      mpi_basic::allgatherv<index_t>(local_sub_fw_degrees.data(), send_count, g.head, counts, displs);
      for (vertex_t u = 0; u < sub_n; ++u) {
        g.head[u + 1] = g.head[u] + g.head[u + 1];
      }
    }
  }

  return PACK(super_ids, g);
}

} // namespace kaspan
