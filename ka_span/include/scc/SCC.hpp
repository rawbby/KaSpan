#pragma once

#include <scc/Common.hpp>
#include <scc/GraphReduction.hpp>
#include <scc/PivotSelection.hpp>
#include <scc/SyncFwBw.hpp>
#include <scc/Trim1.hpp>
#include <util/Result.hpp>

#include <graph/DistributedGraph.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>

#include <algorithm>
#include <cstdio>

template<WorldPartitionConcept Partition>
void
normalize_scc_id(U64Buffer& scc_id, Partition const& part)
{
  for (vertex_t k = 0; k < part.size(); ++k) {
    auto const u = part.select(k);
    if (scc_id[k] == scc_id_singular)
      scc_id[k] = u;
  }
}

template<WorldPartitionConcept Partition>
VoidResult
scc_detection(kamping::Communicator<>& comm, DistributedGraph<Partition> const& graph, U64Buffer& scc_id)
{
  namespace kmp = kamping;

  // scc id contains per local node an id that maps it to an scc
  std::ranges::fill(scc_id.range(), scc_id_undecided);

  auto frontier_kernel = SyncFrontier{ graph.partition };
  RESULT_TRY(auto frontier, SyncAlltoallvBase<SyncFrontier<Partition>>::create(comm, frontier_kernel));
  RESULT_TRY(auto fw_reached, BitBuffer::create(graph.partition.size()));

  // RESULT_TRY(auto edge_comm, SyncEdgeComm<Partition>::create(comm));

  u64 decided_count        = 0;
  u64 global_decided_count = 0;

  // preprocess graph - remove trivial
  trim_1_first(graph, scc_id, decided_count);

  while (global_decided_count != graph.n) {
    std::memset(fw_reached.data(), 0, fw_reached.bytes());

    // run forward backwards search
    IF(NOT(KASPAN_NORMALIZE), const)
    auto root = pivot_selection(comm, graph, scc_id);

    sync_forward_search(comm, graph, frontier, scc_id, fw_reached, root);

    IF(KASPAN_NORMALIZE,
       root = comm.allreduce_single(kmp::send_buf(root), kmp::op(MPI_MIN)));

    sync_backward_search(comm, graph, frontier, scc_id, fw_reached, root, decided_count);

    global_decided_count = comm.allreduce_single(kmp::send_buf(decided_count), kmp::op(MPI_SUM));
  }

  normalize_scc_id(scc_id, graph.partition);
  return VoidResult::success();
}
