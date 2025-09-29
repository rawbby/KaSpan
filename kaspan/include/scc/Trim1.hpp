#pragma once

#include <kamping/communicator.hpp>

#include <cstdint>

template<WorldPartConcept Part>
void
trim_1_first(
  GraphPart<Part> const& graph,
  U64Buffer&                         scc_id,
  u64&                                decided_count)
{
  for (size_t k = 0; k < graph.part.size(); ++k) {
    if (graph.fw_degree(k) == 0 or graph.bw_degree(k) == 0) {
      scc_id[k] = scc_id_singular;
      ++decided_count;
    }
  }
}
