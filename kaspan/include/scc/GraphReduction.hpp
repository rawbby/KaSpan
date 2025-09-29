#pragma once

#include <buffer/Buffer.hpp>
#include <graph/GraphPart.hpp>
#include <graph/Graph.hpp>

#include <kamping/communicator.hpp>

static auto
gfq_origin(
  kamping::Communicator<>& /* comm */,
  GraphPart<> const& /* graph */,
  U64Buffer const& /* scc_id */) -> Result<Graph>
{
  // 1. sync active nodes total

  // u64 local_active_n = 0;
  // for (u64 k = 0; k < graph.part.size(); ++k)
  //   if (scc_id[k] == scc_id_undecided)
  //     ++local_active_n;

  // auto const global_active_n = comm.allreduce_single(kamping::send_buf(local_active_n), kamping::op(MPI_SUM));

  return Graph{};
}
