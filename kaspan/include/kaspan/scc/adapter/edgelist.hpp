#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/util/pp.hpp>

#include <algorithm>

namespace kaspan {

inline void
sorted_edgelist_to_graph(
  vertex_t      n,
  vertex_t      m,
  edge_t const* edgelist,
  index_t*      head,
  vertex_t*     csr)
{
  DEBUG_ASSERT(std::is_sorted(edgelist, edgelist + m, edge_less));
  head[0]     = 0;
  index_t end = 0;
  for (vertex_t u = 0; u < n; ++u) {
    while (end < m && edgelist[end].u == u) {
      csr[end] = edgelist[end].v;
      ++end;
    }
    head[u + 1] = end;
  }
  DEBUG_ASSERT_EQ(end, m);
}

inline void
edgelist_to_graph(
  vertex_t  n,
  vertex_t  m,
  edge_t*   edgelist,
  index_t*  head,
  vertex_t* csr)
{
  std::sort(edgelist, edgelist + m, edge_less);
  sorted_edgelist_to_graph(n, m, edgelist, head, csr);
}

template<part_view_concept Part>
void
sorted_edgelist_to_graph_part(
  Part          part,
  vertex_t      local_m,
  edge_t const* edgelist,
  index_t*      head,
  vertex_t*     csr)
{
  DEBUG_ASSERT(std::is_sorted(edgelist, edgelist + local_m, edge_less));
  auto const local_n = part.local_n();
  head[0]            = 0;
  index_t end        = 0;
  part.each_k([&](auto k) {
    auto const u = part.to_global(k);
    while (end < local_m && edgelist[end].u == u) {
      csr[end] = edgelist[end].v;
      ++end;
    }
    head[k + 1] = end;
  });
  DEBUG_ASSERT_EQ(end, local_m);
}

template<part_view_concept Part>
void
edgelist_to_graph_part(
  Part      part,
  vertex_t  local_m,
  edge_t*   edgelist,
  index_t*  head,
  vertex_t* csr)
{
  std::sort(edgelist, edgelist + local_m, edge_less);
  sorted_edgelist_to_graph_part(part, local_m, edgelist, head, csr);
}

} // namespace kaspan
