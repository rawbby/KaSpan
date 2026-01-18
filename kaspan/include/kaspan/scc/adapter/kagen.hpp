#pragma once

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
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/backward_complement.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <kagen.h>

namespace kaspan {

inline auto
kagen_graph_part(
  char const* generator_args) -> bidi_graph_part<explicit_sorted_part>
{
  kagen::KaGen graph_generator{ mpi_basic::comm_world };
  graph_generator.UseCSRRepresentation();

  auto const ka_graph = graph_generator.GenerateFromOptionString(generator_args);
  ASSERT_IN_RANGE_INCLUSIVE(ka_graph.NumberOfGlobalVertices(), std::numeric_limits<vertex_t>::min(), std::numeric_limits<vertex_t>::max());
  ASSERT_IN_RANGE_INCLUSIVE(ka_graph.NumberOfGlobalEdges(), std::numeric_limits<index_t>::min(), std::numeric_limits<index_t>::max());
  auto const global_n   = integral_cast<vertex_t>(ka_graph.NumberOfGlobalVertices());
  auto const local_n    = integral_cast<vertex_t>(ka_graph.NumberOfLocalVertices());
  auto const local_fw_m = integral_cast<index_t>(ka_graph.NumberOfLocalEdges());
  auto const vertex_end = integral_cast<vertex_t>(ka_graph.vertex_range.second);

  auto g = bidi_graph_part<explicit_sorted_part>{ { global_n, vertex_end }, local_fw_m, 0 };

  if (local_n > 0) {
    auto const& xadj   = ka_graph.xadj;
    auto const& adjncy = ka_graph.adjncy;
    std::copy(xadj.begin(), xadj.end(), g.fw.head);
    std::copy(adjncy.begin(), adjncy.end(), g.fw.csr);
  }

  backward_complement_graph_part(g);
  g.debug_validate();

  return g;
}

} // namespace kaspan
