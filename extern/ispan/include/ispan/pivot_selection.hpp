#pragma once

#include <ispan/util.hpp>

inline kaspan::index_t
pivot_selection(kaspan::vertex_t const* scc_id, kaspan::index_t const* fw_head, kaspan::index_t const* bw_head, kaspan::vertex_t n)
{
  kaspan::index_t  max_degree_product = 0;
  kaspan::vertex_t pivot              = 0;

  for (kaspan::vertex_t u = 0; u < n; ++u) {
    if (scc_id[u] == kaspan::scc_id_undecided) {

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
