#pragma once

#include <buffer/Buffer.hpp>
#include <graph/Base.hpp>
#include <graph/GraphPart.hpp>
#include <graph/Part.hpp>
#include <util/Arithmetic.hpp>

#include <cstdlib>

template<WorldPartConcept Part>
auto
trim_1_first(GraphPart<Part> const& graph, U64Buffer& scc_id) -> std::tuple<u64, Degree>
{
  KASPAN_STATISTIC_SCOPE("trim_1_first");
  size_t decided_count = 0;
  Degree max{};

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    auto const od = graph.fw_degree(k); // out degree
    auto const id = graph.bw_degree(k); // in degree

    if (od == 0 or id == 0) {
      scc_id[k] = scc_id_singular;
      ++decided_count;

    } else if (auto const dp = od * id; dp >= max.degree) { // degree product
      max = { dp, graph.part.to_global(k) };
    }
  }

  KASPAN_STATISTIC_ADD("decision_count", decided_count);
  return std::tuple{ decided_count, max };
}
