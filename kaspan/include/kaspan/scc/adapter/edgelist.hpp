#pragma once

#include <kaspan/debug/assert.hpp>
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
  edge_t*   edgelist,
  index_t*  head,
  vertex_t* csr)
{
  std::sort(edgelist, edgelist + m, edge_less);
  sorted_edgelist_to_graph(n, m, edgelist, head, csr);
}

template<part_concept part_t>
void
sorted_edgelist_to_graph_part(
  part_t const& part,
  vertex_t      local_m,
  edge_t const* edgelist,
  index_t*      head,
  vertex_t*     csr)
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

template<part_concept part_t>
void
edgelist_to_graph_part(
  part_t const& part,
  vertex_t      local_m,
  edge_t*       edgelist,
  index_t*      head,
  vertex_t*     csr)
{
  std::sort(edgelist, edgelist + local_m, edge_less);
  sorted_edgelist_to_graph_part(part, local_m, edgelist, head, csr);
}

} // namespace kaspan
