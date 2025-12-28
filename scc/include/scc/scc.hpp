#pragma once

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <scc/allgather_sub_graph.hpp>
#include <scc/backward_search.hpp>
#include <scc/base.hpp>
#include <scc/ecl_scc_step.hpp>
#include <scc/forward_search.hpp>
#include <scc/pivot_selection.hpp>
#include <scc/trim_1.hpp>
#include <scc/trim_tarjan.hpp>

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

  vertex_t local_decided = 0;

  KASPAN_STATISTIC_PUSH("trim_1");
  // notice: trim_1_first has a side effect by initializing scc_id with scc_id undecided
  // if trim_1_first is removed one has to initialize scc_id with scc_id_undecided manually!
  auto const [trim_1_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
  KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(trim_1_decided, MPI_SUM));
  local_decided += trim_1_decided;
  KASPAN_STATISTIC_POP();

  // fallback to tarjan on single rank
  if (mpi_world_size == 1) {
    tarjan(part, fw_head, fw_csr, [=](auto const* cbeg, auto const* cend) {
      auto const id = *std::min_element(cbeg, cend);
      std::for_each(cbeg, cend, [=](auto const k) {
        scc_id[k] = id;
      });
    },
           SCC_ID_UNDECIDED_FILTER(local_n, scc_id),
           local_decided);
    return;
  }

  auto const decided_threshold = n - (2 * n / mpi_world_size); // as we only gather fw graph we can only reduce to 2 * local_n
  DEBUG_ASSERT_GE(decided_threshold, 0);

  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("forward_backward_search");

    auto frontier   = vertex_frontier::create();
    auto first_root = pivot_selection(max);
    DEBUG_ASSERT_NE(first_root, scc_id_undecided);
    auto       bitvector  = make_bits_clean(local_n);
    auto const first_id   = forward_search(part, fw_head, fw_csr, frontier, scc_id, bitvector, first_root);
    auto const fb_decided = backward_search(part, bw_head, bw_csr, frontier, scc_id, bitvector, first_root, first_id);
    local_decided += fb_decided;

    KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(fb_decided, MPI_SUM));
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  }

  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("ecl");

    auto fw_label     = make_array<vertex_t>(local_n);
    auto bw_label     = make_array<vertex_t>(local_n);
    auto active_array = make_array<vertex_t>(local_n - local_decided);
    auto active       = make_bits_clean(local_n);
    auto changed      = make_bits_clean(local_n);
    auto frontier     = edge_frontier::create();

    do {
      ecl_scc_init_lable(part, fw_label, bw_label);
      local_decided += ecl_scc_step(
        part, fw_head, fw_csr, bw_head, bw_csr, scc_id, fw_label, bw_label, active_array, active, changed, frontier, local_decided);
      // maybe: redistribute graph - sort vertices by ecl label and run trim tarjan (as there is now a lot locality)
    } while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold);

    KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(local_n - local_decided, MPI_SUM));
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  }

  // local_decided += trim_tarjan(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));

  {
    KASPAN_STATISTIC_SCOPE("residual");

    auto [TMP(), sub_ids_inverse, TMP(), sub_n, sub_m, sub_head, sub_csr] =
      allgather_fw_sub_graph(part, local_n - local_decided, fw_head, fw_csr, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    DEBUG_ASSERT_VALID_GRAPH(sub_n, sub_m, sub_head, sub_csr);

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

    KASPAN_STATISTIC_ADD("n", sub_n);
    KASPAN_STATISTIC_ADD("m", sub_m);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(local_n - local_decided, MPI_SUM));
  }
}
