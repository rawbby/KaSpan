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

  auto front          = frontier{ graph.part.local_n() };
  auto message_buffer = vector<vertex_t>{};
  auto vertex_buffer0 = make_array<vertex_t>(graph.part.local_n());
  auto vertex_buffer1 = make_array<vertex_t>(graph.part.local_n());

  KASPAN_STATISTIC_PUSH("trim_ex_first");
  auto local_decided  = trim_1_exhaustive_first(graph, scc_id, vertex_buffer0.data(), vertex_buffer1.data(), front.view<vertex_t>());
  auto global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("select_pivot");
  auto const pivot = select_pivot_from_degree(graph.part, scc_id, vertex_buffer0.data(), vertex_buffer1.data());
  KASPAN_STATISTIC_POP();

  auto bitbuffer0 = make_bits_clean(graph.part.local_n());

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;
  auto active       = make_array<vertex_t>(graph.part.local_n());
  auto is_reached   = make_bits(graph.part.local_n());
  auto is_undecided = make_bits(graph.part.local_n());
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

  vertex_t iterations          = 0;
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;

  KASPAN_STATISTIC_PUSH("color");
  auto label        = make_array<vertex_t>(graph.part.local_n());
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
  KASPAN_STATISTIC_ADD("iterations", iterations);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
