#pragma once

#include <buffer/Buffer.hpp>
#include <comm/MpiBasic.hpp>
#include <graph/GraphPart.hpp>
#include <graph/Part.hpp>
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

template<WorldPartConcept Part>
auto
pivot_selection(kamping::Communicator<>& comm, GraphPart<Part> const& graph, U64Buffer const& scc_id) -> u64
{
  Degree max_degree{};
  for (u64 k = 0; k < graph.part.size(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const out_degree     = graph.fw_degree(k);
      auto const in_degree      = graph.bw_degree(k);
      auto const degree_product = out_degree * in_degree;

      if (degree_product >= max_degree.degree) {
        max_degree.degree = degree_product;
        max_degree.u      = graph.part.select(k);
      }
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
