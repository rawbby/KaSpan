#pragma once

#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/base.hpp>
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

inline index_t
degree(
  index_t const* head,
  vertex_t       u)
{
  return head[u - 1] - head[u];
}

inline index_t
degree(
  vertex_t       n,
  index_t const* head)
{
  if (n > 0) {
    return head[n];
  }
  return 0;
}

template<part_concept Part>
  requires(Part::continuous())
index_t
degree(
  Part const&    part,
  index_t const* head)
{
  return degree(part.local_n(), head);
}

template<part_concept Part>
  requires(not Part::continuous())
index_t
degree(
  Part const&    part,
  index_t const* head)
{
  index_t m = 0;

  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) {
    m += degree(head, k);
  }
  return m;
}

} // namespace kaspan
