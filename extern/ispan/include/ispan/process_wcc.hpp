#pragma once

#include <ispan/util.hpp>

#include <set>

inline void
process_wcc(
  kaspan::vertex_t       n,
  kaspan::vertex_t*      wcc_fq,
  kaspan::vertex_t const* wcc_id,
  kaspan::vertex_t&      wcc_fq_size)
{
  wcc_fq_size = 0;
  for (kaspan::vertex_t i = 0; i < n; ++i)
    if (i == wcc_id[i]) // i is root and every component has exactly one root
      wcc_fq[wcc_fq_size++] = i;
  KASPAN_STATISTIC_ADD("count", wcc_fq_size);
}
