#pragma once

#include <debug/assert.hpp>
#include <scc/base.hpp>
#include <scc/part.hpp>
#include <util/arithmetic.hpp>

#include <algorithm>

inline void
sorted_edgelist_to_graph(
  vertex_t    n,
  vertex_t    m,
  Edge const* edgelist,
  index_t*    head,
  vertex_t*   csr)
{
  DEBUG_ASSERT(std::is_sorted(edgelist, edgelist + m, edge_less));
  head[0]     = 0;
  index_t end = 0;
  for (vertex_t u = 0; u < n; ++u) {
    while (end < m and edgelist[end].u == u) {
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
  Edge*     edgelist,
  index_t*  head,
  vertex_t* csr)
{
  std::sort(edgelist, edgelist + m, edge_less);
  sorted_edgelist_to_graph(n, m, edgelist, head, csr);
}

template<WorldPartConcept Part>
void
sorted_edgelist_to_graph_part(
  Part const& part,
  vertex_t    local_m,
  Edge const* edgelist,
  index_t*    head,
  vertex_t*   csr)
{
  DEBUG_ASSERT(std::is_sorted(edgelist, edgelist + local_m, edge_less));
  auto const local_n = part.local_n();
  head[0]            = 0;
  index_t end        = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u = part.to_global(k);
    while (end < local_m and edgelist[end].u == u) {
      csr[end] = edgelist[end].v;
      ++end;
    }
    head[k + 1] = end;
  }
  DEBUG_ASSERT_EQ(end, local_m);
}

template<WorldPartConcept Part>
void
edgelist_to_graph_part(
  Part const& part,
  vertex_t    local_m,
  Edge*       edgelist,
  index_t*    head,
  vertex_t*   csr)
{
  std::sort(edgelist, edgelist + local_m, edge_less);
  sorted_edgelist_to_graph_part(part, local_m, edgelist, head, csr);
}
