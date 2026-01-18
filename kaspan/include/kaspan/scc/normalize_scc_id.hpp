#pragma once

#include <kaspan/debug/statistic.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/block_cyclic_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/cyclic_part.hpp>
#include <kaspan/graph/explicit_continuous_part.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/graph/trivial_slice_part.hpp>

namespace kaspan {

template<world_part_concept Part>
void
normalize_scc_id(
  Part const& part,
  vertex_t*     scc_id)
{
  KASPAN_STATISTIC_SCOPE("normalize_scc_id");
  // for (vertex_t k = 0; k < part.local_n(); ++k)
  //   if (scc_id[k] == scc_id_singular)
  //     scc_id[k] = part.to_global(k);
}

} // namespace kaspan
