#pragma once

#include <buffer/Buffer.hpp>
#include <comm/MpiBasic.hpp>
#include <graph/DistributedGraph.hpp>
#include <graph/Partition.hpp>
#include <scc/Common.hpp>
#include <util/Arithmetic.hpp>
#include <util/ScopeGuard.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/communicator.hpp>

struct Degree
{
  u64 degree;
  u64 u;
};

template<WorldPartitionConcept Partition>
auto
pivot_selection(kamping::Communicator<>& comm, DistributedGraph<Partition> const& graph, U64Buffer const& scc_id) -> u64
{
  Degree max_degree{};
  for (u64 k = 0; k < graph.partition.size(); ++k) {
    u64 const index = graph.partition.select(k);

    if (scc_id[index] == scc_id_undecided) {
      auto const   out_degree = graph.fw_degree(k);
      auto const   in_degree  = graph.bw_degree(k);
      Degree const degree{ out_degree * in_degree, index };

      if (degree.degree > max_degree.degree)
        max_degree = degree;
    }
  }

  auto global = comm.allreduce_single(
    kamping::send_buf(max_degree),
    kamping::op([](Degree const& lhs, Degree const& rhs) -> Degree const& {
      return lhs.degree > rhs.degree or (lhs.degree == rhs.degree and lhs.u > rhs.u) ? lhs : rhs;
    },
                kamping::ops::commutative));

  return global.u;
}
