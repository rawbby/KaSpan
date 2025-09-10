#pragma once

#include <scc/GraphReduction.hpp>
#include <scc/PivotSelection.hpp>
#include <scc/SyncForwardBackwardSearch.hpp>
#include <scc/Trim1.hpp>
#include <util/Result.hpp>

#include <graph/DistributedGraph.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>

#include <algorithm>
#include <cstdio>

inline void
normalize_scc_id(U64Buffer& scc_id, u64 n)
{
  std::unordered_map<u64, u64> cid;
  for (u64 i = 0; i < n; ++i) {
    if (scc_id[i] == scc_id_singular)
      scc_id[i] = i;

    else {
      auto [it, inserted] = cid.try_emplace(scc_id[i], i);
      if (inserted)
        scc_id[i] = i;
      else // take present id
        scc_id[i] = it->second;
    }
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

    // run forward backwards search
    auto const root = pivot_selection(comm, graph, scc_id);
    sync_forward_search(comm, graph, frontier, scc_id, fw_reached, root);
    sync_backward_search(comm, graph, frontier, scc_id, fw_reached, root, decided_count);

    global_decided_count = comm.allreduce_single(kmp::send_buf(decided_count), kmp::op(MPI_SUM));
  }

  normalize_scc_id(scc_id, graph.n);
  return VoidResult::success();
}
