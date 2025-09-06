#pragma once

#include <buffer/Buffer.hpp>
#include <graph/DistributedGraph.hpp>
#include <graph/Partition.hpp>
#include <scc/Common.hpp>
#include <util/Arithmetic.hpp>
#include <util/MpiTuple.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>

template<WorldPartitionConcept Partition>
auto
pivot_selection(kamping::Communicator<>& comm, DistributedGraph<Partition> const& graph, U64Buffer const& scc_id) -> u64
{
  using Degree = std::tuple<u64, u64>; // (degree, node_id)

  MPI_Datatype MPI_DEGREE_T   = mpi_make_tuple_type<Degree>();
  MPI_Op       MPI_DEGREE_MAX = mpi_make_tuple_max_op<Degree>();
  SCOPE_GUARD(mpi_free(MPI_DEGREE_T, MPI_DEGREE_MAX));

  Degree max_degree{};
  for (u64 k = 0; k < graph.partition.size(); ++k) {
    u64 const index = graph.partition.select(k);

    if (scc_id[index] == SCC_ID_UNDECIDED) { // this local node is active
      auto const   out_degree = graph.fw_degree(k);
      auto const   in_degree  = graph.bw_degree(k);
      Degree const degree{ out_degree * in_degree, index };

      if (mpi_tuple_gt(degree, max_degree))
        max_degree = degree;
    }
  }

  comm.allreduce(
    kamping::send_recv_buf(max_degree),
    kamping::send_recv_count(1),
    kamping::send_recv_type(MPI_DEGREE_T),
    kamping::op(MPI_DEGREE_MAX));

  return std::get<1>(max_degree);
}
