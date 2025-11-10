#pragma once

#include <ispan/util.hpp>

inline index_t
pivot_selection(vertex_t const* scc_id, index_t const* fw_head, index_t const* bw_head, vertex_t n)
{
  KASPAN_STATISTIC_SCOPE("pivot_selection");

  index_t  max_degree_product = 0;
  vertex_t pivot              = 0;

  for (vertex_t u = 0; u < n; ++u) {
    if (scc_id[u] == scc_id_undecided) {

      auto const out_degree = fw_head[u + 1] - fw_head[u];
      auto const in_degree  = bw_head[u + 1] - bw_head[u];

      auto const degree_product = out_degree * in_degree;
      if (degree_product > max_degree_product) {
        max_degree_product = degree_product;
        pivot              = u;
      }
    }
  }

  return pivot;
}
