#pragma once

#include <scc/GraphReduction.hpp>
#include <scc/PivotSelection.hpp>
#include <scc/SyncForwardBackwardSearch.hpp>
#include <scc/color_propagation.h>
#include <scc/trim_1_gfq.h>
#include <util/ErrorCode.hpp>
#include <util/Time.hpp>

#include <graph/DistributedGraph.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <mpi.h>
#include <sys/types.h>

template<WorldPartitionConcept Partition>
VoidResult
scc_detection(kamping::Communicator<>& comm, DistributedGraph<Partition> const& graph)
{
  // scc id contains per local node an id that maps it to an scc
  RESULT_TRY(auto scc_id, U64Buffer::create(graph.partition.n));
  std::ranges::fill(scc_id.range(), SCC_ID_UNDECIDED);

  // frontier is an communicator that abstracts communication for forward and backward searches
  RESULT_TRY(auto frontier, SyncFrontierComm<Partition>::create(comm));
  // forward-reached is an intermediate result from the forward search used by the backward search
  RESULT_TRY(auto fw_reached, BitBuffer::create(graph.partition.size()));

  MPI_Barrier(MPI_COMM_WORLD);

  /* single run to remove huge component */
  {
    // await all ranks before starting the timer

    // 1. trim_1 and sync scc_id
    trim_1_first_and_sync(comm, graph, scc_id);

    // 2. find the pivot across all ranks
    auto const root = pivot_selection(comm, graph, scc_id);

    // 3. forward reachability
    sync_forward_search(comm, graph, frontier, scc_id, fw_reached, root);
    frontier.clear(); // clear for reuse

    // 4. backward reachability
    sync_backward_search(comm, graph, frontier, scc_id, fw_reached, root);
  }

  /* assume tiny graph from now on. replicate and solve everywhere. */
  {
    // 5. double trim and sync scc (non trivial without a lot of communication)
    // trim_1_normal(comm, graph, scc_id);
    // trim_1_normal(comm, graph, scc_id);
    // MPI_Allreduce(MPI_IN_PLACE, scc_id.data(), scc_id.size(), SCC_ID_T, SCC_ID_REDUCE_OP, MPI_COMM_WORLD);

    // // 6. build reduced global graph
    // // (it is expected that the graph is now a lot smaller)
    // gfq_origin(comm, graph, scc_id, small_queue, vert_map, sub_fw_beg, sub_fw_csr, sub_bw_beg, sub_bw_csr);

    // auto const sub_v_count = front_comm[comm.rank()];
    // if (sub_v_count > 0) {
    //   // 7. find weak connected components

    //   i64 wcc_fq_size = 0;
    //   for (i64 i = 0; i < sub_v_count; ++i)
    //     color[i] = i;

    //   coloring_wcc(color, sub_fw_beg, sub_fw_csr, sub_bw_beg, sub_bw_csr, 0, sub_v_count);
    //   process_wcc(0, sub_v_count, wcc_fq, color, wcc_fq_size);
    //   mice_fw_bw(comm, color, scc_id_mice, sub_fw_beg, sub_bw_beg, sub_fw_csr, sub_bw_csr, fw_sa_temp, sub_v_count, wcc_fq, wcc_fq_size);
    //   MPI_Allreduce(MPI_IN_PLACE, scc_id_mice.data(), graph.node_count, MPI_LONG, MPI_MAX, MPI_COMM_WORLD);

    //   for (i64 i = 0; i < sub_v_count; ++i)
    //     scc_id[small_queue[i]] = small_queue[scc_id_mice[i]];
    // }
  }

  return VoidResult::success();
}
