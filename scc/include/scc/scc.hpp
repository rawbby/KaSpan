#pragma once

#include "trim_tarjan.hpp"

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <memory/accessor/bits_accessor.hpp>
#include <scc/allgather_sub_graph.hpp>
#include <scc/backward_search.hpp>
#include <scc/base.hpp>
#include <scc/ecl_scc_step.hpp>
#include <scc/forward_search.hpp>
#include <scc/normalize_scc_id.hpp>
#include <scc/pivot_selection.hpp>
#include <scc/trim_1.hpp>

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
    tarjan(part, fw_head, fw_csr, [=](auto const* cbeg, auto const* cend) {
      auto const id = *std::min_element(cbeg, cend);
      std::for_each(cbeg, cend, [=](auto const k) {
        scc_id[k] = id;
      });
    });
    return;
  }

  std::fill(scc_id, scc_id + local_n, scc_id_undecided);

  vertex_t   local_decided     = 0;
  auto const decided_threshold = n - (2 * n / mpi_world_size); // as we only gather fw graph we can only reduce to 2 * local_n
  DEBUG_ASSERT_GE(decided_threshold, 0);

  auto [trim_1_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
  local_decided += trim_1_decided;

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    auto frontier   = vertex_frontier::create();
    auto first_root = pivot_selection(max);
    DEBUG_ASSERT_NE(first_root, scc_id_undecided);
    auto [bb, bitvector] = BitsAccessor::create_clean(local_n);
    auto const first_id  = forward_search(part, fw_head, fw_csr, frontier, scc_id, bitvector, first_root);
    local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, bitvector, first_root, first_id);
  }
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("ecl");
  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {

    auto    ecl_buffer = make_buffer<vertex_t, vertex_t,vertex_t ,vertex_t, u64>(local_n,local_n,local_n,local_n, (local_n + 63) >> 6);
    void*     ecl_memory   = ecl_buffer.data();
    vertex_t* ecl_fw_label = borrow<vertex_t>(ecl_memory, local_n);
    vertex_t* ecl_bw_label = borrow<vertex_t>(ecl_memory, local_n);
    auto      active_stack = StackAccessor<vertex_t>::borrow(ecl_memory, local_n);
    auto      active       = BitsAccessor::borrow(ecl_memory, local_n);
    auto      changed      = BitsAccessor::borrow(ecl_memory, local_n);
    auto      frontier     = edge_frontier::create();

    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

    do {
      ecl_scc_init_lable(part, ecl_fw_label, ecl_bw_label);
      local_decided += ecl_scc_step(
        part,
        fw_head,
        fw_csr,
        bw_head,
        bw_csr,
        scc_id,
        ecl_fw_label,
        ecl_bw_label,
        active_stack,
        active,
        changed,
        frontier);
      // maybe: redistribute graph - sort vertices by ecl label and run trim tarjan (as there is now a lot locality)
    } while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold);
  }
  KASPAN_STATISTIC_POP();

  // local_decided += trim_tarjan(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));

  {
    KASPAN_STATISTIC_SCOPE("residual");
    auto [TMP(), sub_ids_inverse, TMP(), sub_n, sub_m, sub_head, sub_csr] =
      allgather_fw_sub_graph(part, local_n - local_decided, fw_head, fw_csr, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));

    DEBUG_ASSERT_VALID_GRAPH(sub_n, sub_m, sub_head, sub_csr);
    KASPAN_STATISTIC_ADD("n", sub_n);
    KASPAN_STATISTIC_ADD("m", sub_m);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    if (sub_n) {
      tarjan(sub_n, sub_head, sub_csr, [&](auto const* beg, auto const* end) {
        auto const min_u = sub_ids_inverse[*std::min_element(beg, end)];
        for (auto sub_u : std::span{ beg, end }) {
          auto const u = sub_ids_inverse[sub_u];
          if (part.has_local(u)) {
            scc_id[part.to_local(u)] = min_u;
          }
        }
      });
    }
  }
}
