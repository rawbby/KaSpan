#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>
#include <kaspan/scc/backward_search.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/color_scc_step.hpp>
#include <kaspan/scc/forward_search.hpp>
#include <kaspan/scc/pivot_selection.hpp>
#include <kaspan/scc/trim_1.hpp>
#include <kaspan/scc/trim_tarjan.hpp>
#include <kaspan/util/return_pack.hpp>

#include <algorithm>

namespace kaspan {

template<world_part_concept part_t>
void
scc(part_t const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(part, bw_head, bw_csr);
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n;
  auto const local_n = part.local_n();

  vertex_t local_decided  = 0;
  vertex_t global_decided = 0;

  // notice: trim_1_first has a side effect by initializing scc_id with scc_id undecided
  // if trim_1_first is removed one has to initialize scc_id with scc_id_undecided manually!
  auto const max = [&] {
    KASPAN_STATISTIC_SCOPE("trim_1_first");
    auto const [trim_1_decided, trim_1_max] = trim_1_first(part, fw_head, bw_head, scc_id);
    local_decided += trim_1_decided;
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("decided_count", global_decided);
    return trim_1_max;
  }();

  // fallback to tarjan on single rank
  if (mpi_basic::world_size == 1) {
    auto const undecided_filter = SCC_ID_UNDECIDED_FILTER(local_n, scc_id);
    auto const on_scc_range     = [=](vertex_t* beg, vertex_t* end) {
      auto const id = *std::min_element(beg, end);
      std::for_each(beg, end, [=](auto const k) {
        scc_id[k] = id;
      });
    };

    tarjan(part, fw_head, fw_csr, on_scc_range, undecided_filter, local_decided);
    return;
  }

  auto const decided_threshold = n - (2 * n / mpi_basic::world_size); // as we only gather fw graph we can only reduce to 2 * local_n
  DEBUG_ASSERT_GE(decided_threshold, 0);

  if (global_decided < decided_threshold) {
    {
      KASPAN_STATISTIC_SCOPE("forward_backward_search");

      auto frontier = vertex_frontier::create(local_n);
      auto pivot    = pivot_selection(max);

      auto bitvector = make_bits_clean(local_n);
      forward_search(part, fw_head, fw_csr, frontier, scc_id, static_cast<bits_accessor>(bitvector), pivot);
      local_decided += backward_search(part, bw_head, bw_csr, frontier, scc_id, static_cast<bits_accessor>(bitvector), pivot);

      auto const prev_global_decided = global_decided;

      global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
      KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
      KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    }
    {
      KASPAN_STATISTIC_SCOPE("trim_1_fw_bw");
      auto const trim_1_decided = trim_1(part, fw_head, fw_csr, bw_head, bw_csr, scc_id);
      local_decided += trim_1_decided;
      auto const prev_global_decided = global_decided;
      global_decided                 = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
      KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
    }
  }

  if (global_decided < decided_threshold) {
    {
      KASPAN_STATISTIC_SCOPE("color");

      auto colors       = make_array<vertex_t>(local_n);
      auto active_array = make_array<vertex_t>(local_n - local_decided);
      auto active       = make_bits_clean(local_n);
      auto changed      = make_bits_clean(local_n);
      auto frontier     = edge_frontier::create(local_n);

      auto const prev_global_decided = global_decided;

      do {
        color_scc_init_label(part, colors.data());
        local_decided += color_scc_step(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, colors.data(), active_array.data(), static_cast<bits_accessor>(active), static_cast<bits_accessor>(changed), frontier, local_decided);
        global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
        // maybe: redistribute graph - sort vertices by color and run trim tarjan (as there is now a lot locality)
      } while (global_decided < decided_threshold);

      KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
      KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    }
    {
      KASPAN_STATISTIC_SCOPE("trim_1_color");
      auto const trim_1_decided = trim_1(part, fw_head, fw_csr, bw_head, bw_csr, scc_id);
      local_decided += trim_1_decided;
      auto const prev_global_decided = global_decided;
      global_decided                 = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
      KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
    }
  }

  {
    KASPAN_STATISTIC_SCOPE("residual");
    auto const undecided_filter = SCC_ID_UNDECIDED_FILTER(local_n, scc_id);

    auto [TMP(), sub_ids_inverse, TMP(), sub_n, sub_m, sub_head, sub_csr] = allgather_fw_sub_graph(part, local_n - local_decided, fw_head, fw_csr, undecided_filter);

    if (sub_n) {
      tarjan(sub_n, sub_head, sub_csr, [&](auto const* beg, auto const* end) {
        auto const id = sub_ids_inverse[*std::min_element(beg, end)];
        for (auto sub_u : std::span{ beg, end }) {
          auto const u = sub_ids_inverse[sub_u];
          if (part.has_local(u)) {
            scc_id[part.to_local(u)] = id;
            ++local_decided;
          }
        }
      });
    }

    KASPAN_STATISTIC_ADD("n", sub_n);
    KASPAN_STATISTIC_ADD("m", sub_m);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

    auto const prev_global_decided = global_decided;

    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);

    DEBUG_ASSERT_EQ(global_decided, part.n);
  }
}

} // namespace kaspan
