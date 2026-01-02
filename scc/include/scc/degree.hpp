#pragma once

#include <scc/base.hpp>
#include <scc/part.hpp>

inline index_t
degree(index_t const* head, vertex_t u)
{
  return head[u - 1] - head[u];
}

inline index_t
degree(vertex_t n, index_t const* head)
{
  if (n > 0) { return head[n]; }
  return 0;
}

template<WorldPartConcept Part>
  requires(Part::continuous)
index_t
degree(Part const& part, index_t const* head)
{
  return degree(part.local_n(), head);
}

template<WorldPartConcept Part>
  requires(not Part::continuous)
index_t
degree(Part const& part, index_t const* head)
{
  index_t m = 0;

  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) { m += degree(head, k); }
  return m;
}
