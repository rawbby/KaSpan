#pragma once

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
#include <scc/wcc.hpp>
#include <util/result.hpp>
#include <util/scope_guard.hpp>

#include <algorithm>
#include <cstdio>

template<WorldPartConcept Part>
VoidResult
scc(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n;
  auto const local_n = part.local_n();

  KASPAN_STATISTIC_PUSH("alloc");
  std::ranges::fill(scc_id, scc_id + local_n, scc_id_undecided);
  auto frontier         = vertex_frontier::create();
  auto [bb, fw_reached] = BitAccessor::create_clean(local_n);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  auto [local_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
  {
    auto root = pivot_selection(max);
    forward_search(part, fw_head, fw_csr, frontier, scc_id, fw_reached, root);
    local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, fw_reached, root);
  }

  auto const decided_threshold = n / mpi_world_size;
  while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    fw_reached.clear(local_n);
    auto root = pivot_selection(part, fw_head, bw_head, scc_id);
    forward_search(part, fw_head, fw_csr, frontier, scc_id, fw_reached, root);
    local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, fw_reached, root);
  }

  {
    KASPAN_STATISTIC_SCOPE("residual");
    auto sub_graph = allgather_sub_graph(part, fw_head, fw_csr, bw_head, bw_csr, [&scc_id](vertex_t k) {
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

      auto const wcc_count = wcc(sub_graph.n, sub_graph.fw_head, sub_graph.fw_csr, wcc_id);
      residual_scc(
        wcc_id,
        wcc_count,
        sub_scc_id,
        sub_graph.n,
        sub_graph.fw_head,
        sub_graph.fw_csr,
        sub_graph.bw_head,
        sub_graph.bw_csr,
        sub_graph.ids_inverse);

      KASPAN_STATISTIC_SCOPE("post_processing");
      mpi_basic_allreduce_inplace(sub_scc_id, sub_graph.n, MPI_MIN);
      for (vertex_t sub_v = 0; sub_v < sub_graph.n; ++sub_v) {
        auto const v = sub_graph.ids_inverse[sub_v];
        if (part.has_local(v)) {
          scc_id[part.to_local(v)] = sub_scc_id[sub_v];
        }
      }
    }
  }

  KASPAN_STATISTIC_SCOPE("post_processing");
  normalize_scc_id(part, scc_id);
  return VoidResult::success();
}
