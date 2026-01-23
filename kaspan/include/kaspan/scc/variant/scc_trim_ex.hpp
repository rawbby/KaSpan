#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/backward_search.hpp>
#include <kaspan/scc/forward_search.hpp>
#include <kaspan/scc/pivot.hpp>
#include <kaspan/scc/trim_1_exhaustive.hpp>
#include <kaspan/memory/accessor/stack.hpp>

namespace kaspan {

template<part_view_concept Part>
void
scc_trim_ex(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  auto front          = frontier{ graph.part.local_n() };
  auto vertex_buffer0 = make_array<vertex_t>(graph.part.local_n());
  auto vertex_buffer1 = make_array<vertex_t>(graph.part.local_n());

  auto outdegree = vertex_buffer0.data();
  auto indegree  = vertex_buffer1.data();

  auto       decided_stack = make_stack<vertex_t>(graph.part.local_n());
  auto const on_decision   = [&](vertex_t k) { decided_stack.push(k); };

  KASPAN_STATISTIC_PUSH("trim_ex_first");
  auto local_decided  = trim_1_exhaustive_first(graph, scc_id, outdegree, indegree, front.view<vertex_t>());
  auto global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("select_pivot");
  auto const pivot = select_pivot_from_degree(graph.part, scc_id, outdegree, indegree);
  KASPAN_STATISTIC_POP();

  auto bitbuffer0 = make_bits_clean(graph.part.local_n());
  auto bitbuffer1 = make_bits_clean(graph.part.local_n());

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  forward_search(graph, front.view<vertex_t>(), scc_id, bitbuffer0.data(), bitbuffer1.data(), pivot, local_decided);
  auto const fwbw_local_decided  = backward_search(graph, front.view<vertex_t>(), scc_id, bitbuffer0.data(), bitbuffer1.data(), pivot, local_decided, on_decision);
  auto const fwbw_global_decided = mpi_basic::allreduce_single(fwbw_local_decided, mpi_basic::sum);
  local_decided += fwbw_local_decided;
  global_decided += fwbw_global_decided;
  KASPAN_STATISTIC_ADD("local_decided", fwbw_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", fwbw_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_ex");
  auto local_decided  = trim_1_exhaustive(graph, scc_id, outdegree.data(), indegree.data(), front.view<vertex_t>(), decided_stack.begin(), decided_stack.end());
  decided_stack.clear();
  auto global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  bitbuffer0.clear(graph.part.local_n());
  bitbuffer1.clear(graph.part.local_n());

  auto colors = make_array<vertex_t>(graph.part.local_n());
  auto active = make_array<vertex_t>(graph.part.local_n() - local_decided);

  vertex_t iterations          = 0;
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;

  KASPAN_STATISTIC_PUSH("color_trim_ex");
  do {
    local_decided += color_scc_step(graph, scc_id, colors.data(), active.data(), bitbuffer0.data(), bitbuffer1.data(), front.view<edge_t>(), local_decided, on_decision);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    ++iterations;

    if (global_decided >= graph.part.n()) break;

    auto local_decided  = trim_1_exhaustive(graph, scc_id, outdegree.data(), indegree.data(), front.view<vertex_t>(), decided_stack.begin(), decided_stack.end());
    decided_stack.clear();
    auto global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

    if (global_decided >= graph.part.n()) break;

    local_decided +=
      color_scc_step(graph.inverse_view(), scc_id, colors.data(), active.data(), bitbuffer0.data(), bitbuffer1.data(), front.view<edge_t>(), local_decided, on_decision);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    ++iterations;

    if (global_decided >= graph.part.n()) break;

    auto local_decided  = trim_1_exhaustive(graph, scc_id, outdegree.data(), indegree.data(), front.view<vertex_t>(), decided_stack.begin(), decided_stack.end());
    decided_stack.clear();
    auto global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

  } while (global_decided < graph.part.n());
  KASPAN_STATISTIC_ADD("iterations", iterations);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
