#pragma once

#include "memory/bit_accessor.hpp"

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <scc/allgather_graph.hpp>
#include <scc/async/backward_search.hpp>
#include <scc/async/forward_search.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <scc/normalize_scc_id.hpp>
#include <scc/pivot_selection.hpp>
#include <scc/residual_scc.hpp>
#include <scc/trim_1.hpp>
#include <scc/wcc.hpp>
#include <util/result.hpp>

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/indirection.hpp>
#include <briefkasten/noop_indirection.hpp>
#include <briefkasten/queue_builder.hpp>

#include <algorithm>
#include <cstdio>

namespace async {

template<typename IndirectionScheme>
auto
make_briefkasten()
{
  if constexpr (std::same_as<IndirectionScheme, briefkasten::NoopIndirectionScheme>) {
    return briefkasten::BufferedMessageQueueBuilder<vertex_t>().build();
  } else {
    return briefkasten::IndirectionAdapter{
      briefkasten::BufferedMessageQueueBuilder<vertex_t>()
        .with_merger(briefkasten::aggregation::EnvelopeSerializationMerger{})
        .with_splitter(briefkasten::aggregation::EnvelopeSerializationSplitter<vertex_t>{})
        .build(),
      IndirectionScheme{ MPI_COMM_WORLD }
    };
  }
}

template<typename IndirectionScheme, WorldPartConcept Part>
VoidResult
scc(auto const& graph, vertex_t* scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");
  KASPAN_STATISTIC_PUSH("alloc");
  std::ranges::fill(scc_id.range(), scc_id_undecided);
  auto brief_queue = make_briefkasten<IndirectionScheme>();
  auto fw_reached, BitAccessor::create(graph.part.local_n());
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  auto [local_decided, max] = trim_1_first(graph, scc_id);

  {
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    auto root = pivot_selection(comm, max);
    async::forward_search(comm, graph, brief_queue, scc_id, fw_reached, root);
    local_decided += async::backward_search(graph, brief_queue, scc_id, fw_reached, root);
  }

  auto const decided_threshold = graph.n / mpi_world_size;
  while (.allreduce_single(::send_buf(local_decided), MPI_SUM) < decided_threshold) {
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    auto root = pivot_selection(graph, scc_id);
    async::forward_search(graph, brief_queue, scc_id, fw_reached, root);
    local_decided += async::backward_search(graph, brief_queue, scc_id, fw_reached, root);
  }

  {
    KASPAN_STATISTIC_SCOPE("residual");
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
      KASPAN_STATISTIC_SCOPE("post_processing");
      MPI_Allreduce(MPI_IN_PLACE, sub_scc_id.data(), sub_graph.n, mpi_scc_id_t, MPI_MIN, MPI_COMM_WORLD);
      for (u64 sub_v = 0; sub_v < sub_graph.n; ++sub_v) {
        auto const v = sub_ids_inverse[sub_v];
        if (graph.part.has_local(v)) {
          auto const k = graph.part.to_local(v);
          scc_id[k]    = sub_scc_id[sub_v];
        }
      }
    }
  }

  KASPAN_STATISTIC_SCOPE("post_processing");
  normalize_scc_id(scc_id, graph.part);
  return VoidResult::success();
}

}
