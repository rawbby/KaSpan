#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>
#include <kaspan/scc/backward_search.hpp>
#include <kaspan/scc/color_scc_step.hpp>
#include <kaspan/scc/forward_search.hpp>
#include <kaspan/scc/pivot.hpp>
#include <kaspan/scc/trim_1_exhaustive.hpp>
#include <kaspan/scc/trim_tarjan.hpp>
#include <kaspan/util/pp.hpp>

#include <algorithm>

namespace kaspan {

template<part_concept Part>
void
scc(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  graph.debug_validate();
  auto const& part = *graph.part;
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n();
  auto const m       = mpi_basic::allreduce_single(graph.local_fw_m, mpi_basic::sum);
  auto const local_n = part.local_n();

  auto frontier  = vertex_frontier<>::create(local_n);
  auto outdegree = make_array<vertex_t>(local_n);
  auto indegree  = make_array<vertex_t>(local_n);

  auto const undecided_filter = SCC_ID_UNDECIDED_FILTER(local_n, scc_id);

  auto       decided_stack = make_stack<vertex_t>(local_n);
  auto const on_decision   = [&](vertex_t k) { decided_stack.push(k); };

  // notice: trim_1_exhaustive_first has a side effect by initializing scc_id with scc_id undecided
  // if trim_1_exhaustive_first is removed one has to initialize scc_id with scc_id_undecided manually!
  KASPAN_STATISTIC_PUSH("trim_1_exhaustive_first");
  vertex_t local_decided  = trim_1_exhaustive_first(graph, scc_id, outdegree.data(), indegree.data(), frontier);
  vertex_t global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", global_decided);
  KASPAN_STATISTIC_POP();

  // fallback to tarjan on single rank
  if (mpi_basic::world_size <= 1) {
    auto const on_scc = [=](auto* beg, auto* end) {
      auto const id = *std::min_element(beg, end);
      std::for_each(beg, end, [=](auto const k) { scc_id[k] = id; });
    };
    tarjan(graph.fw_view(), on_scc, undecided_filter, local_decided);
    return;
  }

  auto const decided_threshold = n - (n / mpi_basic::world_size);
  DEBUG_ASSERT_GE(decided_threshold, 0);

  if (global_decided < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("forward_backward_search");

    // as degree arrays are up to date we can use this more
    // accurate degree information for a better pivot selection
    auto pivot = select_pivot_from_degree(part, scc_id, outdegree.data(), indegree.data());

    auto bitvector = make_bits_clean(local_n);

    forward_search(graph.fw_view(), frontier, scc_id, bitvector.data(), pivot);
    local_decided += backward_search(graph.bw_view(), frontier, scc_id, bitvector.data(), pivot, on_decision);

    auto const prev_global_decided = global_decided;

    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  }

  if (global_decided < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("color");

    auto colors        = make_array<vertex_t>(local_n);
    auto active_array  = make_array<vertex_t>(local_n - local_decided);
    auto active        = make_bits_clean(local_n);
    auto changed       = make_bits_clean(local_n);
    auto frontier_edge = edge_frontier::create(local_n);

    vertex_t iterations = 0;

    auto const prev_global_decided = global_decided;

    do {

      local_decided += color_scc_step(graph, scc_id, colors.data(), active_array.data(), active.data(), changed.data(), frontier_edge, local_decided, on_decision);
      global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
      ++iterations;

      if (global_decided < decided_threshold) break;

      local_decided += color_scc_step(graph.inverse_view(), scc_id, colors.data(), active_array.data(), active.data(), changed.data(), frontier_edge, local_decided, on_decision);
      global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
      ++iterations;

    } while (global_decided < decided_threshold);

    KASPAN_STATISTIC_ADD("iterations", iterations);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  }

  if (!decided_stack.empty()) {
    KASPAN_STATISTIC_SCOPE("trim_1_exhaustive");
    local_decided += trim_1_exhaustive(graph, scc_id, outdegree.data(), indegree.data(), frontier, decided_stack.begin(), decided_stack.end());
    decided_stack.clear();

    auto const prev_global_decided = global_decided;

    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  }

  {
    KASPAN_STATISTIC_SCOPE("residual");
    auto [sub_ids_inverse, sub_g] = allgather_fw_sub_graph_with_degrees(part, local_n - local_decided, graph.fw.head, graph.fw.csr, outdegree.data(), undecided_filter);
    if (sub_g.n) {
      tarjan(sub_g.view(), [&](auto const* beg, auto const* end) {
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

    KASPAN_STATISTIC_ADD("n", sub_g.n);
    KASPAN_STATISTIC_ADD("m", sub_g.m);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

    auto const prev_global_decided = global_decided;

    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);

    DEBUG_ASSERT_EQ(global_decided, part.n());
  }
}

} // namespace kaspan
