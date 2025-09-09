#pragma once

#include <kamping/communicator.hpp>

#include <cstdint>

template<WorldPartitionConcept Partition>
void
trim_1_first(
  DistributedGraph<Partition> const& graph,
  U64Buffer&                         scc_id)
{
  for (size_t k = 0; k < graph.partition.size(); ++k) {
    if (graph.fw_degree(k) == 0 or graph.bw_degree(k) == 0) {
      scc_id[k]            = SCC_ID_SINGLE;
    }
  }
}
