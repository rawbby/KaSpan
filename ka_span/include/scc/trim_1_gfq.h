#pragma once

#include <kamping/communicator.hpp>

#include <cstdint>
#include <vector>

template<WorldPartitionConcept Partition>
void
trim_1_first_and_sync(kamping::Communicator<>& comm, DistributedGraph<Partition> const& graph, U64Buffer& scc_id)
{
  // for all local nodes with a degree of 0 (out or in) set scc_id to -1
  // this version does not consider self loops as a special case!

  if constexpr (std::is_same_v<TrivialSlicePartition, Partition> or std::is_same_v<BalancedSlicePartition, Partition>) {

    // the slice partitions have a layout that allows a special all gather version that is expected to be more performant
    for (size_t k = 0; k < graph.partition.size(); ++k)
      if (graph.fw_degree(k) == 0 or graph.bw_degree(k) == 0)
        scc_id[graph.partition.select(k)] = SCC_ID_SINGLE;

    MPI_Allgather(MPI_IN_PLACE, 0, SCC_ID_T, scc_id.data(), graph.partition.size(), SCC_ID_T, MPI_COMM_WORLD);

  } else {

    std::ranges::fill(scc_id.range(), 0);

    // for every other partition use the fallback reduce version
    for (size_t k = 0; k < graph.partition.size(); ++k)
      if (graph.fw_degree(k) == 0 or graph.bw_degree(k) == 0)
        scc_id[graph.partition.select(k)] = SCC_ID_SINGLE;

    MPI_Allreduce(MPI_IN_PLACE, scc_id.data(), scc_id.size(), SCC_ID_T, SCC_ID_REDUCE_OP, MPI_COMM_WORLD);
  }
}

template<WorldPartitionConcept Partition>
void
trim_1_normal(kamping::Communicator<>& comm, DistributedGraph<Partition> const& graph, U64Buffer& scc_id)
{
  using namespace kamping; // todo use comm

  // for all local nodes with a degree of 0 (out or in) set scc_id to -1
  // this version does consider self loops as a special case!

  // utility: check for a node if it has an active degree
  auto const active_degree = [&](u64 k, auto const& head, auto const& csr) {
    auto const csr_begin  = head.get(k);
    auto const csr_length = head.get(k + 1) - csr_begin;
    for (auto const v : csr.range(csr_begin, csr_length))
      if (not graph.partition.contains(v) or scc_id[v] == SCC_ID_UNDECIDED)
        return true;
    return false;
  };

  for (size_t k = 0; k < graph.partition.size(); ++k) {
    auto const u = graph.partition.select(k);
    if (scc_id[u] == SCC_ID_UNDECIDED) {

      // index is active and has no active out degree -> scc
      if (not active_degree(u, k, graph.fw_head, graph.fw_csr))
        scc_id[u] = SCC_ID_SINGLE;

      // index is active and has no active in degree -> scc
      else if (not active_degree(u, k, graph.bw_head, graph.bw_csr))
        scc_id[u] = SCC_ID_SINGLE;
    }
  }
}

