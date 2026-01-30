#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/backward_search.hpp>
#include <kaspan/scc/forward_search.hpp>
#include <kaspan/scc/tarjan.hpp>
#include <kaspan/scc/trim_1_local.hpp>

namespace kaspan {

template<part_view_concept Part>
void
scc_hpc_like(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");

  KASPAN_STATISTIC_PUSH("trim_1_first");
  auto [local_decided, pivot] = trim_1_first(graph, scc_id);
  auto global_decided         = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("pivot", pivot);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  auto front      = frontier{ graph.part.local_n() };
  auto message_buffer = vector<vertex_t>{};
  auto bitbuffer0 = make_bits_clean(graph.part.local_n());
  auto bitbuffer1 = make_bits_clean(graph.part.local_n());

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  forward_search(graph, front.view<vertex_t>(), message_buffer, scc_id, bitbuffer0.data(), pivot);
  auto const fwbw_local_decided  = backward_search(graph, front.view<vertex_t>(), message_buffer, scc_id, bitbuffer0.data(), pivot);
  auto const fwbw_global_decided = mpi_basic::allreduce_single(fwbw_local_decided, mpi_basic::sum);
  local_decided += fwbw_local_decided;
  global_decided += fwbw_global_decided;
  KASPAN_STATISTIC_ADD("local_decided", fwbw_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", fwbw_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", fwbw_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  auto colors = make_array<vertex_t>(graph.part.local_n());
  bitbuffer0.clear(graph.part.local_n());
  bitbuffer1.clear(graph.part.local_n());

  vertex_t iterations          = 0;
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;

  KASPAN_STATISTIC_PUSH("color");
  do {
    local_decided += color_scc_step(graph, front.view<edge_t>(), message_buffer, bitbuffer0.data(), bitbuffer1.data(), colors.data(), scc_id);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    ++iterations;

    if (global_decided >= graph.part.n()) break;

    local_decided +=
      color_scc_step(graph.inverse_view(), front.view<edge_t>(), message_buffer, bitbuffer0.data(), bitbuffer1.data(), colors.data(), scc_id);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    ++iterations;
  } while (global_decided < graph.part.n());
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
