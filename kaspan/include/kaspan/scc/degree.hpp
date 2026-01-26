#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/util/integral_cast.hpp>

namespace kaspan {

inline vertex_t
degree(
  index_t const* head,
  vertex_t       u)
{
  return integral_cast<vertex_t>(head[u + 1] - head[u]);
}

inline index_t
total_degree(
  vertex_t       n,
  index_t const* head)
{
  if (n > 0) {
    return head[n];
  }
  return 0;
}

template<part_view_concept Part>
  requires(Part::continuous)
index_t
degree(
  Part           part,
  index_t const* head)
{
  return total_degree(part.local_n(), head);
}

template<part_view_concept Part>
  requires(!Part::continuous)
index_t
degree(
  Part           part,
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
