#pragma once

#include "trim_tarjan.hpp"

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <memory/bit_accessor.hpp>
#include <scc/allgather_sub_graph.hpp>
#include <scc/backward_search.hpp>
#include <scc/base.hpp>
#include <scc/forward_search.hpp>
#include <scc/normalize_scc_id.hpp>
#include <scc/pivot_selection.hpp>
#include <scc/residual_scc.hpp>
#include <scc/trim_1.hpp>
#include <util/result.hpp>
#include <util/scope_guard.hpp>

#include <algorithm>
#include <cstdio>

template<bool use_trim_tarjan = false, WorldPartConcept Part>
VoidResult
scc(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n;
  auto const local_n = part.local_n();

  KASPAN_STATISTIC_PUSH("alloc");
  std::ranges::fill(scc_id, scc_id + local_n, scc_id_undecided);
  auto frontier         = vertex_frontier::create();
  auto [bb, fw_reached] = BitAccessor::create(local_n);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  vertex_t   local_decided     = 0;
  auto const decided_threshold = n / mpi_world_size;

  // refactor ugly tarjan switch
  if (use_trim_tarjan) {
    auto const trim_tarjan_decided = trim_tarjan(part, fw_head, fw_csr, bw_head, bw_csr, scc_id);
    local_decided += trim_tarjan_decided;

    do {
      fw_reached.clear(local_n);
      auto root = pivot_selection(part, fw_head, bw_head, scc_id);
      DEBUG_ASSERT_NE(root, scc_id_undecided);
      auto const id = forward_search(part, fw_head, fw_csr, frontier, scc_id, fw_reached, root);
      KASPAN_STATISTIC_ADD("root", root);
      KASPAN_STATISTIC_ADD("root_id", id);
      local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, fw_reached, root, id);
    } while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold);

  } else {
    auto [trim_1_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
    local_decided += trim_1_decided;

    auto first_root = pivot_selection(max);
    DEBUG_ASSERT_NE(first_root, scc_id_undecided);
    fw_reached.clear(local_n);
    auto const first_id = forward_search(part, fw_head, fw_csr, frontier, scc_id, fw_reached, first_root);
    KASPAN_STATISTIC_ADD("root", first_root);
    KASPAN_STATISTIC_ADD("root_id", first_id);
    local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, fw_reached, first_root, first_id);

    while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
      auto root = pivot_selection(part, fw_head, bw_head, scc_id);
      DEBUG_ASSERT_NE(root, scc_id_undecided);
      fw_reached.clear(local_n);
      auto const id = forward_search(part, fw_head, fw_csr, frontier, scc_id, fw_reached, root);
      KASPAN_STATISTIC_ADD("root", root);
      KASPAN_STATISTIC_ADD("root_id", id);
      local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, fw_reached, root, id);
    }
  }

  auto const trim_tarjan_decided = trim_tarjan(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, [=, &scc_id](vertex_t k) {
    DEBUG_ASSERT_GE(k, 0);
    DEBUG_ASSERT_LT(k, local_n);
    return scc_id[k] == scc_id_undecided;
  });
  local_decided += trim_tarjan_decided;

  {
    KASPAN_STATISTIC_SCOPE("residual");
    auto sub_graph = allgather_sub_graph(part, fw_head, fw_csr, bw_head, bw_csr, [=, &scc_id](vertex_t k) {
      DEBUG_ASSERT_GE(k, 0);
      DEBUG_ASSERT_LT(k, local_n);
      return scc_id[k] == scc_id_undecided;
    });

    KASPAN_STATISTIC_ADD("n", sub_graph.n);
    KASPAN_STATISTIC_ADD("m", sub_graph.m);
    auto  buffer     = Buffer::create(2 * page_ceil<vertex_t>(sub_graph.n));
    auto* memory     = buffer.data();
    auto* wcc_id     = ::borrow<vertex_t>(memory, sub_graph.n);
    auto* sub_scc_id = ::borrow<vertex_t>(memory, sub_graph.n);
    for (vertex_t i = 0; i < sub_graph.n; ++i) {
      wcc_id[i]     = i;
      sub_scc_id[i] = scc_id_undecided;
    }
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    if (sub_graph.n) {
      tarjan(sub_graph.n, sub_graph.fw_head, sub_graph.fw_csr, [&](vertex_t* beg, vertex_t* end) {
        vertex_t min_v = std::numeric_limits<vertex_t>::max();
        for (auto& c : std::span{ beg, end }) {
          c     = sub_graph.ids_inverse[c];
          min_v = std::min(min_v, c);
        }
        for (auto const& c : std::span{ beg, end }) {
          if (part.has_local(c)) {
            scc_id[part.to_local(c)] = min_v;
          }
        }
      });
    }
  }

  KASPAN_STATISTIC_SCOPE("post_processing");
  normalize_scc_id(part, scc_id);
  return VoidResult::success();
}
