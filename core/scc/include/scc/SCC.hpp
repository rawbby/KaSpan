#pragma once

#include <scc/Common.hpp>
#include <scc/FwBwPart.hpp>
#include <scc/GraphReduction.hpp>
#include <scc/PivotSelection.hpp>
#include <scc/SyncFwBw.hpp>
#include <scc/Trim1.hpp>
#include <util/Result.hpp>
#include <wcc/WCC.hpp>

#include <graph/AllGatherGraph.hpp>
#include <graph/GraphPart.hpp>

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/indirection.hpp>
#include <briefkasten/noop_indirection.hpp>
#include <briefkasten/queue_builder.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>
#include <kamping/measurements/timer.hpp>

#include <algorithm>
#include <cstdio>

template<typename IndirectionScheme>
auto
make_async_queue(kamping::Communicator<>& comm)
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

template<WorldPartConcept Part>
void
normalize_scc_id(U64Buffer& scc_id, Part const& part)
{
  for (vertex_t k = 0; k < part.size(); ++k) {
    auto const u = part.select(k);
    if (scc_id[k] == scc_id_singular)
      scc_id[k] = u;
  }
}

template<WorldPartConcept Part>
VoidResult
sync_scc_detection(kamping::Communicator<>& comm, GraphPart<Part> const& graph, U64Buffer& scc_id, std::vector<std::pair<std::string, std::string>>* stats = nullptr)
{
  namespace kmp = kamping;
  auto& t       = kmp::measurements::timer();

  t.start("kaspan_alloc");
  // scc id contains per local node an id that maps it to an scc
  std::ranges::fill(scc_id.range(), scc_id_undecided);

  auto frontier_kernel = SyncFrontier{ graph.part };
  RESULT_TRY(auto frontier, SyncAlltoallvBase<SyncFrontier<Part>>::create(comm, frontier_kernel));

  RESULT_TRY(auto fw_reached, BitBuffer::create(graph.part.size()));
  u64 decided_count        = 0;
  u64 global_decided_count = 0;
  t.stop();

  // preprocess graph - remove trivial
  t.start("kaspan_trim1_first");
  trim_1_first(graph, scc_id, decided_count);
  t.stop();

  size_t const trim1reduction = global_decided_count;

  // reduce until the global graph is smaller than the partition but at least once
  // - in most cases there is a single big component here
  // - in every other case this one is nessesary as one can not gather before because of space requirements
  t.start("kaspan_fwbw");
  size_t fwbwc = 0; // count
  do {              // NOLINT(*-avoid-do-while)
    ++fwbwc;
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    IF(NOT(KASPAN_NORMALIZE), const)
    auto root = pivot_selection(comm, graph, scc_id);

    sync_forward_search(comm, graph, frontier, scc_id, fw_reached, root);
    IF(KASPAN_NORMALIZE, root = comm.allreduce_single(kmp::send_buf(root), kmp::op(MPI_MIN)));
    sync_backward_search(comm, graph, frontier, scc_id, fw_reached, root, decided_count);

    global_decided_count = comm.allreduce_single(kmp::send_buf(decided_count), kmp::op(MPI_SUM));
  } while (global_decided_count < graph.n / comm.size());
  t.stop();

  size_t const fwbw_reduction = global_decided_count;

  t.start("kaspan_residual");
  RESULT_TRY(auto sub_graph_ids_inverse_and_ids, AllGatherSubGraph(graph, [&scc_id](vertex_t k) {
               return scc_id[k] == scc_id_undecided;
             }));
  auto [sub_graph, sub_ids_inverse, sub_ids] = std::move(sub_graph_ids_inverse_and_ids);
  t.stop();

  t.start("kaspan_alloc");
  RESULT_TRY(auto wcc_id, U64Buffer::create(sub_graph.n));
  RESULT_TRY(auto sub_scc_id, U64Buffer::create(sub_graph.n));
  for (u64 i = 0; i < sub_graph.n; ++i) {
    wcc_id[i]     = i;
    sub_scc_id[i] = scc_id_undecided;
  }
  t.stop_and_add();

  if (sub_graph.n) {
    t.start("kaspan_wcc");
    auto const wcc_count = wcc_detection(sub_graph, wcc_id);
    t.stop();

    t.start("kaspan_wcc_fwbw");
    FwBwPart(comm, wcc_id, wcc_count, sub_scc_id, sub_graph, sub_ids_inverse);
    t.stop();

    t.start("kaspan_post");
    MPI_Allreduce(
      MPI_IN_PLACE, sub_scc_id.data(), sub_graph.n, mpi_scc_id_t, MPI_MIN, MPI_COMM_WORLD);

    for (u64 sub_v = 0; sub_v < sub_graph.n; ++sub_v) {
      auto const v = sub_ids_inverse[sub_v];

      if (graph.part.contains(v)) {
        auto const k = graph.part.rank(v);
        scc_id[k]    = sub_scc_id[sub_v];
      }
    }
  } else {
    t.start("kaspan_post");
  }

  normalize_scc_id(scc_id, graph.part);
  kmp::measurements::timer().stop();

  stats->emplace_back("fwbw_count", std::to_string(fwbwc));
  stats->emplace_back("trim1reduction", std::to_string(trim1reduction));
  stats->emplace_back("fwbw_reduction", std::to_string(fwbw_reduction));

  return VoidResult::success();
}

template<typename IndirectionScheme, WorldPartConcept Part>
VoidResult
async_scc_detection(kamping::Communicator<>& comm, GraphPart<Part> const& graph, U64Buffer& scc_id, std::vector<std::pair<std::string, std::string>>* stats = nullptr)
{
  namespace kmp = kamping;
  auto& t       = kmp::measurements::timer();

  t.start("kaspan_alloc");
  // scc id contains per local node an id that maps it to an scc
  std::ranges::fill(scc_id.range(), scc_id_undecided);

  auto frontier_kernel = SyncFrontier{ graph.part };
  RESULT_TRY(auto frontier, SyncAlltoallvBase<SyncFrontier<Part>>::create(comm, frontier_kernel));

  auto brief_queue = make_async_queue<IndirectionScheme>(comm);

  RESULT_TRY(auto fw_reached, BitBuffer::create(graph.part.size()));
  u64 decided_count        = 0;
  u64 global_decided_count = 0;
  t.stop();

  // preprocess graph - remove trivial
  t.start("kaspan_trim1_first");
  trim_1_first(graph, scc_id, decided_count);
  t.stop();

  size_t const trim1reduction = global_decided_count;

  // reduce until the global graph is smaller than the partition but at least once
  // - in most cases there is a single big component here
  // - in every other case this one is nessesary as one can not gather before because of space requirements
  t.start("kaspan_fwbw");
  size_t fwbwc = 0; // count
  do {              // NOLINT(*-avoid-do-while)
    ++fwbwc;
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    IF(NOT(KASPAN_NORMALIZE), const)
    auto root = pivot_selection(comm, graph, scc_id);

    async_forward_search(comm, graph, brief_queue, scc_id, fw_reached, root);
    IF(KASPAN_NORMALIZE, root = comm.allreduce_single(kmp::send_buf(root), kmp::op(MPI_MIN)));
    async_backward_search(comm, graph, brief_queue, scc_id, fw_reached, root, decided_count);

    global_decided_count = comm.allreduce_single(kmp::send_buf(decided_count), kmp::op(MPI_SUM));
  } while (global_decided_count < graph.n / comm.size());
  t.stop();

  size_t const fwbw_reduction = global_decided_count;

  t.start("kaspan_residual");
  RESULT_TRY(auto sub_graph_ids_inverse_and_ids, AllGatherSubGraph(graph, [&scc_id](vertex_t k) {
               return scc_id[k] == scc_id_undecided;
             }));
  auto [sub_graph, sub_ids_inverse, sub_ids] = std::move(sub_graph_ids_inverse_and_ids);
  t.stop();

  t.start("kaspan_alloc");
  RESULT_TRY(auto wcc_id, U64Buffer::create(sub_graph.n));
  RESULT_TRY(auto sub_scc_id, U64Buffer::create(sub_graph.n));
  for (u64 i = 0; i < sub_graph.n; ++i) {
    wcc_id[i]     = i;
    sub_scc_id[i] = scc_id_undecided;
  }
  t.stop_and_add();

  if (sub_graph.n) {
    t.start("kaspan_wcc");
    auto const wcc_count = wcc_detection(sub_graph, wcc_id);
    t.stop();

    t.start("kaspan_wcc_fwbw");
    FwBwPart(comm, wcc_id, wcc_count, sub_scc_id, sub_graph, sub_ids_inverse);
    t.stop();

    t.start("kaspan_post");
    MPI_Allreduce(
      MPI_IN_PLACE, sub_scc_id.data(), sub_graph.n, mpi_scc_id_t, MPI_MIN, MPI_COMM_WORLD);

    for (u64 sub_v = 0; sub_v < sub_graph.n; ++sub_v) {
      auto const v = sub_ids_inverse[sub_v];

      if (graph.part.contains(v)) {
        auto const k = graph.part.rank(v);
        scc_id[k]    = sub_scc_id[sub_v];
      }
    }
  } else {
    t.start("kaspan_post");
  }

  normalize_scc_id(scc_id, graph.part);
  kmp::measurements::timer().stop();

  stats->emplace_back("fwbw_count", std::to_string(fwbwc));
  stats->emplace_back("trim1reduction", std::to_string(trim1reduction));
  stats->emplace_back("fwbw_reduction", std::to_string(fwbw_reduction));

  return VoidResult::success();
}
