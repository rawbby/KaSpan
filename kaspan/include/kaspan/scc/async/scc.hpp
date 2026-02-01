#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/scc/async/multi_pivot_search.hpp>
#include <kaspan/scc/async/pivot_search.hpp>
#include <kaspan/scc/trim_1_local.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>

#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/queue_builder.hpp>

#include <cstdio>

namespace kaspan::async {

template<part_view_concept Part>
void
scc(
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

  KASPAN_STATISTIC_PUSH("trim_1_first");
  auto       is_undecided = make_bits_filled(graph.part.local_n());
  auto const pivot        = trim_1_first(graph, is_undecided.data(), on_decision);
  global_decided          = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto active         = make_array<vertex_t>(graph.part.local_n());
  auto in_active      = make_bits(graph.part.local_n());
  {
    auto front = briefkasten::BufferedMessageQueueBuilder<vertex_t>{}.build();
    async::forward_backward_search(graph, front, active.data(), in_active.data(), is_undecided.data(), pivot, on_decision);
  }
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  trim_1_normal(graph, is_undecided.data(), on_decision);
  trim_1_normal(graph, is_undecided.data(), on_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("color");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto label          = make_array<vertex_t>(graph.part.local_n());
  {
    auto front = briefkasten::BufferedMessageQueueBuilder<edge_t>{}.build();
    do {
      async::label_search(graph, front, label.data(), active.data(), in_active.data(), is_undecided.data(), on_decision);
      global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    } while (global_decided < graph.part.n());
  }
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan::async
