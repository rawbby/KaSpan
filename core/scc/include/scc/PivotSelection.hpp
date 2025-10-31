#pragma once

#include <graph/Part.hpp>
#include <scc/Common.hpp>
#include <util/Arithmetic.hpp>

#include <kamping/communicator.hpp>

inline auto
pivot_selection(kamping::Communicator<>& comm, Degree max_degree) -> u64
{
  return comm.allreduce_single(kamping::send_buf(max_degree), kamping::op(Degree::max, kamping::ops::commutative)).u;
}

template<WorldPartConcept Part>
auto
pivot_selection(kamping::Communicator<>& comm, GraphPart<Part> const& graph, U64Buffer const& scc_id) -> u64
{
  Degree max_degree{};
  for (u64 k = 0; k < graph.part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const out_degree     = graph.fw_degree(k);
      auto const in_degree      = graph.bw_degree(k);
      auto const degree_product = out_degree * in_degree;

      if (degree_product >= max_degree.degree) {
        max_degree.degree = degree_product;
        max_degree.u      = graph.part.to_global(k);
      }
    }
  }
  return pivot_selection(comm, max_degree);
}
