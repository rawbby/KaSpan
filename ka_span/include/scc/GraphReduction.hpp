#pragma once

#include <buffer/Buffer.hpp>
#include <graph/DistributedGraph.hpp>
#include <graph/Graph.hpp>
#include <scc/Common.hpp>
#include <util/Arithmetic.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>

static auto
gfq_origin(
  kamping::Communicator<>&  comm,
  DistributedGraph<> const& graph,
  U64Buffer const&          scc_id) -> Result<Graph>
{
  // 1. sync active nodes total

  u64 local_active_n = 0;
  for (u64 k = 0; k < graph.partition.size(); ++k)
    if (scc_id[k] == scc_id_undecided)
      ++local_active_n;

  auto const global_active_n = comm.allreduce_single(kamping::send_buf(local_active_n), kamping::op(MPI_SUM));


}
