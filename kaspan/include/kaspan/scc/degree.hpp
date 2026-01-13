#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

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

template<world_part_concept part_t>
  requires(part_t::continuous)
index_t
degree(
  part_t const&  part,
  index_t const* head)
{
  return degree(part.local_n(), head);
}

template<world_part_concept part_t>
  requires(not part_t::continuous)
index_t
degree(
  part_t const&  part,
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
