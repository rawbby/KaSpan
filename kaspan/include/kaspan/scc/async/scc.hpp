#pragma once

#include <kamping/mpi_datatype.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits.hpp>
#include <kaspan/memory/accessor/vector.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/scc/async/backward_search.hpp>
#include <kaspan/scc/async/color_scc_step.hpp>
#include <kaspan/scc/async/forward_search.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/indirection.hpp>
#include <briefkasten/noop_indirection.hpp>
#include <briefkasten/queue_builder.hpp>

#include <algorithm>
#include <cstdio>

namespace kamping {
template<>
struct mpi_type_traits<kaspan::edge_t>
{
  static constexpr bool has_to_be_committed = false;
  static MPI_Datatype   data_type()
  {
    return kaspan::mpi_edge_t;
  }
};
}

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

  auto message_buffer = vector<vertex_t>{ graph.part.local_n() };

  {
    KASPAN_STATISTIC_PUSH("forward_backward_search");
    vertex_t prev_local_decided  = local_decided;
    vertex_t prev_global_decided = global_decided;
    auto     vertex_frontier     = briefkasten::BufferedMessageQueueBuilder<vertex_t>{}.build();
    auto     bitbuffer0          = make_bits_clean(graph.part.local_n());
    async::forward_search(graph, vertex_frontier, message_buffer, scc_id, bitbuffer0.data(), pivot);
    local_decided += async::backward_search(graph, vertex_frontier, message_buffer, scc_id, bitbuffer0.data(), pivot);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
    KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    KASPAN_STATISTIC_POP();
  }

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("color");
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;
  auto     message_queue       = briefkasten::BufferedMessageQueueBuilder<edge_t>{}.build();
  auto     colors              = make_array<vertex_t>(graph.part.local_n());
  do {

    local_decided += async::color_scc_step(graph, message_queue, message_buffer, colors.data(), scc_id);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

    if (global_decided >= graph.part.n()) break;

    local_decided += async::color_scc_step(graph.inverse_view(), message_queue, message_buffer, colors.data(), scc_id);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

  } while (global_decided < graph.part.n());
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan::async
