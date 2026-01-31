#pragma once

#include <kamping/mpi_datatype.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/bits.hpp>
#include <kaspan/memory/accessor/stack.hpp>
#include <kaspan/memory/accessor/vector.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/scc/trim_1_local.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/indirection.hpp>
#include <briefkasten/noop_indirection.hpp>
#include <briefkasten/queue_builder.hpp>

#include <algorithm>
#include <cstdio>

#include <kaspan/scc/async/forward_backward_search.hpp>
#include <kaspan/scc/async/label_search.hpp>

namespace kaspan::async {

template<part_view_concept Part>
void
scc(
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
  auto vertex_storage0 = make_array<vertex_t>(graph.part.local_n());
  auto bits_storage0   = make_bits(graph.part.local_n());
  auto is_undecided    = make_bits(graph.part.local_n());
  is_undecided.set_each(graph.part.local_n(), [&](auto k) { return scc_id[k] == scc_id_undecided; }); // todo: use everywhere
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;
  {
    auto vertex_frontier = briefkasten::BufferedMessageQueueBuilder<vertex_t>{}.build();
    async::forward_backward_search(graph, vertex_frontier, vertex_storage0.data(), bits_storage0.data(), is_undecided.data(), pivot, [&](auto k, auto id) {
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

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  local_decided += trim_1_normal(graph, scc_id);
  local_decided += trim_1_normal(graph, scc_id);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("color");
  auto vertex_storage1 = make_array<vertex_t>(graph.part.local_n());
  prev_local_decided   = local_decided;
  prev_global_decided  = global_decided;
  is_undecided.set_each(graph.part.local_n(), [&](auto k) { return scc_id[k] == scc_id_undecided; }); // todo: use everywhere
  {
    auto label_frontier = briefkasten::BufferedMessageQueueBuilder<edge_t>{}.build();
    do {
      async::label_search(graph, label_frontier, vertex_storage0.data(), vertex_storage1.data(), bits_storage0.data(), is_undecided.data(), [&](auto k, auto id) {
        scc_id[k] = id;
        ++local_decided;
      });
      global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    } while (global_decided < graph.part.n());
  }
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan::async
