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
#include <kaspan/scc/color_scc_step.hpp>
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

template<typename indirection_scheme_t>
auto
make_briefkasten_vertex()
{
  if constexpr (std::same_as<indirection_scheme_t, briefkasten::NoopIndirectionScheme>) {
    return briefkasten::BufferedMessageQueueBuilder<vertex_t>().build();
  } else {
    return briefkasten::IndirectionAdapter{ briefkasten::BufferedMessageQueueBuilder<vertex_t>()
                                              .with_merger(briefkasten::aggregation::EnvelopeSerializationMerger{})
                                              .with_splitter(briefkasten::aggregation::EnvelopeSerializationSplitter<vertex_t>{})
                                              .build(),
                                            indirection_scheme_t{ mpi_basic::comm_world } };
  }
}

template<typename indirection_scheme_t>
auto
make_briefkasten_edge()
{
  if constexpr (std::same_as<indirection_scheme_t, briefkasten::NoopIndirectionScheme>) {
    return briefkasten::BufferedMessageQueueBuilder<edge_t>().build();
  } else {
    return briefkasten::IndirectionAdapter{ briefkasten::BufferedMessageQueueBuilder<edge_t>().build(), indirection_scheme_t{ mpi_basic::comm_world } };
  }
}

template<typename indirection_scheme_t,
         part_view_concept Part>
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

  auto active     = vector<vertex_t>{ graph.part.local_n() };
  auto bitbuffer0 = make_bits_clean(graph.part.local_n());

  {
    KASPAN_STATISTIC_PUSH("forward_backward_search");
    vertex_t prev_local_decided  = local_decided;
    vertex_t prev_global_decided = global_decided;
    auto     vertex_frontier     = make_briefkasten_vertex<indirection_scheme_t>();
    auto     bitbuffer1          = make_bits_clean(graph.part.local_n());
    async::forward_search(graph, vertex_frontier, scc_id, bitbuffer0.data(), active, pivot);
    mpi_basic::barrier();
    local_decided += async::backward_search(graph, vertex_frontier, scc_id, bitbuffer0.data(), active, pivot);
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
  auto     edge_frontier       = make_briefkasten_edge<indirection_scheme_t>();
  auto     colors              = make_array<vertex_t>(graph.part.local_n());
  bitbuffer0.clear(graph.part.local_n());
  do {

    color_scc_init_label(graph.part, colors.data());
    local_decided += async::color_scc_step(graph, edge_frontier, scc_id, colors.data(), active, bitbuffer0.data());
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

    if (global_decided >= graph.part.n()) break;

    color_scc_init_label(graph.part, colors.data());
    local_decided += async::color_scc_step(graph, edge_frontier, scc_id, colors.data(), active, bitbuffer0.data());
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

  } while (global_decided < graph.part.n());
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan::async
