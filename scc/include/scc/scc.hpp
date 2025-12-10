#pragma once

#include "trim_tarjan.hpp"

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <memory/bit_accessor.hpp>
#include <scc/allgather_sub_graph.hpp>
#include <scc/backward_search.hpp>
#include <scc/base.hpp>
#include <scc/ecl_scc_step.hpp>
#include <scc/forward_search.hpp>
#include <scc/normalize_scc_id.hpp>
#include <scc/pivot_selection.hpp>
#include <scc/trim_1.hpp>
#include <util/scope_guard.hpp>

#include <algorithm>
#include <cstdio>

template<WorldPartConcept Part>
void
scc(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(part, bw_head, bw_csr);
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n;
  auto const local_n = part.local_n();

  // fallback to tarjan on single rank
  if (mpi_world_size == 1) {
    return tarjan(part, fw_head, fw_csr, [&](auto const* cbeg, auto const* cend) {
      auto const id = *std::min_element(cbeg, cend);
      std::for_each(cbeg, cend, [&](auto const k) {
        scc_id[k] = id;
      });
    });
  }

  KASPAN_STATISTIC_PUSH("alloc");
  std::ranges::fill(scc_id, scc_id + local_n, scc_id_undecided);
  auto [bb, bitvector] = BitAccessor::create(local_n);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  vertex_t   local_decided     = 0;
  auto const decided_threshold = n - n / mpi_world_size;
  DEBUG_ASSERT_GT(decided_threshold, 0);

  auto [trim_1_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
  local_decided += trim_1_decided;

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  {
    auto frontier   = vertex_frontier::create();
    auto first_root = pivot_selection(max);
    DEBUG_ASSERT_NE(first_root, scc_id_undecided);
    bitvector.clear(local_n);
    auto const first_id = forward_search(part, fw_head, fw_csr, frontier, scc_id, bitvector, first_root);
    local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, bitvector, first_root, first_id);
  }
  KASPAN_STATISTIC_POP();

  // old logic: over and over apply fw/bw search - was slow, as only a single scc if found per iteration
  // while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
  //   auto root = pivot_selection(part, fw_head, bw_head, scc_id);
  //   DEBUG_ASSERT_NE(root, scc_id_undecided);
  //   fw_reached.clear(local_n);
  //   auto const id = forward_search(part, fw_head, fw_csr, frontier, scc_id, fw_reached, root);
  //   local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, fw_reached, root, id);
  // }

  KASPAN_STATISTIC_PUSH("ecl");
  // assume huge scc component is resolved for power law graph
  // now switch to grid grap strategy to reduce one scc component per wcc
  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    Buffer    ecl_buffer{ 2 * local_n * sizeof(vertex_t) };
    void*     ecl_memory   = ecl_buffer.data();
    vertex_t* ecl_fw_label = borrow<vertex_t>(ecl_memory, local_n);
    vertex_t* ecl_bw_label = borrow<vertex_t>(ecl_memory, local_n);
    auto      frontier     = edge_frontier::create();

    do {
      ecl_scc_init_lable(part, ecl_fw_label, ecl_bw_label);
      local_decided += ecl_scc_step(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, ecl_fw_label, ecl_bw_label, bitvector, frontier);
      // maybe: redistribute graph - sort vertices by ecl label and run trim tarjan (as there is now a lot locality)
    } while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold);
  }
  KASPAN_STATISTIC_POP();

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
    DEBUG_ASSERT_VALID_GRAPH(sub_graph.n, sub_graph.m, sub_graph.fw_head, sub_graph.fw_csr);

    KASPAN_STATISTIC_ADD("n", sub_graph.n);
    KASPAN_STATISTIC_ADD("m", sub_graph.m);
    // auto  buffer     = Buffer(page_ceil<vertex_t>(sub_graph.n));
    // auto* memory     = buffer.data();
    // auto* sub_scc_id = borrow<vertex_t>(memory, sub_graph.n);
    // std::fill(sub_scc_id, sub_scc_id + sub_graph.n, scc_id_undecided);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    if (sub_graph.n) {
      tarjan(sub_graph.n, sub_graph.fw_head, sub_graph.fw_csr, [&](auto const* beg, auto const* end) {
        auto const min_u = sub_graph.ids_inverse[*std::min_element(beg, end)];
        std::for_each(beg, end, [&](auto const sub_u) {
          auto const u = sub_graph.ids_inverse[sub_u];
          if (part.has_local(u)) {
            scc_id[part.to_local(u)] = min_u;
          }
        });
      });
    }
  }

  KASPAN_STATISTIC_SCOPE("post_processing");
  normalize_scc_id(part, scc_id);
}
