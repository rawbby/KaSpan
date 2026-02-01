#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/pivot.hpp>
#include <kaspan/scc/trim_1_exhaustive.hpp>

namespace kaspan {

template<part_view_concept Part>
void
scc_hpc_trim_ex(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");
  vertex_t local_decided       = 0;
  vertex_t global_decided      = 0;
  vertex_t prev_local_decided  = 0;
  vertex_t prev_global_decided = 0;

  auto const on_decision = [&](auto k, auto id) {
    scc_id[k] = id;
    ++local_decided;
  };

  KASPAN_STATISTIC_PUSH("trim_ex_first");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto front          = frontier{ graph.part.local_n() };
  auto vertex_buffer0 = make_array<vertex_t>(graph.part.local_n());
  auto vertex_buffer1 = make_array<vertex_t>(graph.part.local_n());
  auto is_undecided   = make_bits_filled(graph.part.local_n());
  trim_1_exhaustive_first(graph, is_undecided.data(), vertex_buffer0.data(), vertex_buffer1.data(), front.view<vertex_t>(), on_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("select_pivot");
  auto const pivot = select_pivot_from_degree(graph.part, is_undecided.data(), vertex_buffer0.data(), vertex_buffer1.data());
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto bits_buffer0   = make_bits(graph.part.local_n());
  forward_backward_search(graph, front.view<vertex_t>(), vertex_buffer0.data(), bits_buffer0.data(), is_undecided.data(), pivot, on_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("color");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto bits_buffer1   = make_bits(graph.part.local_n());
  do {
    label_search(graph, front.view<edge_t>(), vertex_buffer1.data(), vertex_buffer0.data(), bits_buffer0.data(), bits_buffer1.data(), is_undecided.data(), on_decision);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  } while (global_decided < graph.part.n());
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
