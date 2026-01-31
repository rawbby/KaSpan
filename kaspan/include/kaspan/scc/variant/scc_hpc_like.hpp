#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
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

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  auto     front               = frontier{ graph.part.local_n() };
  auto     message_buffer      = vector<vertex_t>{};
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;
  auto     active              = make_array<vertex_t>(graph.part.local_n());
  auto     is_reached          = make_bits(graph.part.local_n());
  auto     is_undecided        = make_bits(graph.part.local_n());
  {
    is_undecided.set_each(graph.part.local_n(), [&](auto k) { return scc_id[k] == scc_id_undecided; });
    forward_backward_search(graph, front.view<vertex_t>(), active.data(), is_reached.data(), is_undecided.data(), pivot, [&](auto k, auto id) {
      scc_id[k] = id;
      ++local_decided;
    });
  }
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("color");
  vertex_t iterations = 0;
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto   label      = make_array<vertex_t>(graph.part.local_n());
  auto has_changed  = make_bits_clean(graph.part.local_n());
      is_undecided.set_each(graph.part.local_n(), [&](auto k) { return scc_id[k] == scc_id_undecided; });
  do {
    label_search(graph, front.view<edge_t>(), label.data(), active.data(), is_reached.data(), has_changed.data(), is_undecided.data(), [&](auto k, auto id) {
      scc_id[k] = id;
      ++local_decided;
    });
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
