#pragma once

#include <debug/Process.hpp>
#include <debug/Statistic.hpp>
#include <debug/Time.hpp>
#include <graph/AllGatherGraph.hpp>
#include <graph/GraphPart.hpp>
#include <scc/Common.hpp>
#include <scc/GraphReduction.hpp>
#include <scc/NormalizeSccId.hpp>
#include <scc/PivotSelection.hpp>
#include <scc/ResidualScc.hpp>
#include <scc/Trim1.hpp>
#include <scc/Wcc.hpp>
#include <scc/async/BackwardSearch.hpp>
#include <scc/async/ForwardSearch.hpp>
#include <util/Result.hpp>

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/indirection.hpp>
#include <briefkasten/noop_indirection.hpp>
#include <briefkasten/queue_builder.hpp>

#include <kamping/communicator.hpp>

#include <algorithm>
#include <cstdio>

namespace async {

template<typename IndirectionScheme>
auto
make_briefkasten(kamping::Communicator<>& comm)
{
  if constexpr (std::same_as<IndirectionScheme, briefkasten::NoopIndirectionScheme>) {
    return briefkasten::BufferedMessageQueueBuilder<vertex_t>().build();
  } else {
    return briefkasten::IndirectionAdapter{
      briefkasten::BufferedMessageQueueBuilder<vertex_t>()
        .with_merger(briefkasten::aggregation::EnvelopeSerializationMerger{})
        .with_splitter(briefkasten::aggregation::EnvelopeSerializationSplitter<vertex_t>{})
        .build(),
      IndirectionScheme{ comm.mpi_communicator() }
    };
  }
}

template<typename IndirectionScheme, WorldPartConcept Part>
VoidResult
scc(kamping::Communicator<>& comm, GraphPart<Part> const& graph, U64Buffer& scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");
  KASPAN_STATISTIC_PUSH("alloc");
  std::ranges::fill(scc_id.range(), scc_id_undecided);
  auto brief_queue = make_briefkasten<IndirectionScheme>(comm);
  RESULT_TRY(auto fw_reached, BitBuffer::create(graph.part.size()));
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  auto [local_decided, max] = trim_1_first(graph, scc_id);

  {
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    auto root = pivot_selection(comm, max);
    async::forward_search(comm, graph, brief_queue, scc_id, fw_reached, root);
    local_decided += async::backward_search(graph, brief_queue, scc_id, fw_reached, root);
  }

  auto const decided_theshold = graph.n / comm.size();
  while (comm.allreduce_single(kamping::send_buf(local_decided), kamping::op(MPI_SUM)) < decided_theshold) {
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    auto root = pivot_selection(comm, graph, scc_id);
    async::forward_search(comm, graph, brief_queue, scc_id, fw_reached, root);
    local_decided += async::backward_search(graph, brief_queue, scc_id, fw_reached, root);
  }

  KASPAN_STATISTIC_PUSH("residual");
  RESULT_TRY(auto sub_graph_ids_inverse_and_ids, AllGatherSubGraph(graph, [&scc_id](vertex_t k) {
               return scc_id[k] == scc_id_undecided;
             }));
  auto [sub_graph, sub_ids_inverse, sub_ids] = std::move(sub_graph_ids_inverse_and_ids);
  KASPAN_STATISTIC_ADD("n", sub_graph.n);
  KASPAN_STATISTIC_ADD("m", sub_graph.m);
  RESULT_TRY(auto wcc_id, U64Buffer::create(sub_graph.n));
  RESULT_TRY(auto sub_scc_id, U64Buffer::create(sub_graph.n));
  for (u64 i = 0; i < sub_graph.n; ++i) {
    wcc_id[i]     = i;
    sub_scc_id[i] = scc_id_undecided;
  }
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  if (sub_graph.n) {
    auto const wcc_count = wcc(sub_graph, wcc_id);
    residual_scc(comm, wcc_id, wcc_count, sub_scc_id, sub_graph, sub_ids_inverse);
    // todo replace with all to all v?
    KASPAN_STATISTIC_SCOPE("residual_post_processing");
    MPI_Allreduce(MPI_IN_PLACE, sub_scc_id.data(), sub_graph.n, mpi_scc_id_t, MPI_MIN, MPI_COMM_WORLD);
    for (u64 sub_v = 0; sub_v < sub_graph.n; ++sub_v) {
      auto const v = sub_ids_inverse[sub_v];
      if (graph.part.contains(v)) {
        auto const k = graph.part.rank(v);
        scc_id[k]    = sub_scc_id[sub_v];
      }
    }
  }
  KASPAN_STATISTIC_POP();
  KASPAN_STATISTIC_SCOPE("post_processing");
  normalize_scc_id(scc_id, graph.part);
  return VoidResult::success();
}

}
