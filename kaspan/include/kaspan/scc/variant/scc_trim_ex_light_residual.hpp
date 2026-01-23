#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>

namespace kaspan {

template<part_view_concept Part>
void
scc(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
}

} // namespace kaspan
