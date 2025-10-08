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

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>
#include <kamping/measurements/timer.hpp>

#include <algorithm>
#include <cstdio>

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
scc_detection(kamping::Communicator<>& comm, GraphPart<Part> const& graph, U64Buffer& scc_id)
{
  namespace kmp = kamping;

  kamping::measurements::timer().start("kaspan_alloc");
  // scc id contains per local node an id that maps it to an scc
  std::ranges::fill(scc_id.range(), scc_id_undecided);
  auto frontier_kernel = SyncFrontier{ graph.part };
  RESULT_TRY(auto frontier, SyncAlltoallvBase<SyncFrontier<Part>>::create(comm, frontier_kernel));
  RESULT_TRY(auto fw_reached, BitBuffer::create(graph.part.size()));
  u64 decided_count        = 0;
  u64 global_decided_count = 0;
  kamping::measurements::timer().stop();

  kamping::measurements::timer().start("kaspan_trim1_first");
  // preprocess graph - remove trivial
  trim_1_first(graph, scc_id, decided_count);
  kamping::measurements::timer().stop();

  kamping::measurements::timer().start("kaspan_fwbw");
  // reduce until the global graph is smaller than the partition
  while (not global_decided_count or global_decided_count < graph.n / comm.size()) {
    std::memset(fw_reached.data(), 0, fw_reached.bytes());
    IF(NOT(KASPAN_NORMALIZE), const)
    auto root = pivot_selection(comm, graph, scc_id);
    sync_forward_search(comm, graph, frontier, scc_id, fw_reached, root);
    IF(KASPAN_NORMALIZE,
       root = comm.allreduce_single(kmp::send_buf(root), kmp::op(MPI_MIN)));
    sync_backward_search(comm, graph, frontier, scc_id, fw_reached, root, decided_count);
    global_decided_count = comm.allreduce_single(kmp::send_buf(decided_count), kmp::op(MPI_SUM));
  }
  kamping::measurements::timer().stop();

  kamping::measurements::timer().start("kaspan_residual");
  RESULT_TRY(auto sub_graph_ids_inverse_and_ids, AllGatherSubGraph(graph, [&scc_id](vertex_t k) {
               return scc_id[k] == scc_id_undecided;
             }));
  auto [sub_graph, sub_ids_inverse, sub_ids] = std::move(sub_graph_ids_inverse_and_ids);
  kamping::measurements::timer().stop();

  kamping::measurements::timer().start("kaspan_alloc");
  RESULT_TRY(auto wcc_id, U64Buffer::create(sub_graph.n));
  RESULT_TRY(auto sub_scc_id, U64Buffer::create(sub_graph.n));
  for (u64 i = 0; i < sub_graph.n; ++i) {
    wcc_id[i]     = i;
    sub_scc_id[i] = scc_id_undecided;
  }
  kamping::measurements::timer().stop_and_add();

  if (sub_graph.n) {
    kamping::measurements::timer().start("kaspan_wcc");
    auto const wcc_count = wcc_detection(sub_graph, wcc_id);
    kamping::measurements::timer().stop();

    kamping::measurements::timer().start("kaspan_wcc_fwbw");
    FwBwPart(comm, wcc_id, wcc_count, sub_scc_id, sub_graph, sub_ids_inverse);
    kamping::measurements::timer().stop();

    kamping::measurements::timer().start("kaspan_post");
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
    kamping::measurements::timer().start("kaspan_post");
  }

  normalize_scc_id(scc_id, graph.part);
  kmp::measurements::timer().stop();

  return VoidResult::success();
}
