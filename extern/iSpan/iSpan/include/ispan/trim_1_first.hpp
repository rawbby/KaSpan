#pragma once

#include <ispan/util.hpp>

inline void
trim_1_first(vertex_t* scc_id, index_t const* fw_head, index_t const* bw_head, vertex_t local_beg, vertex_t local_end)
{
  for (auto u = local_beg; u < local_end; ++u)
    if (fw_head[u + 1] - fw_head[u] == 0 or bw_head[u + 1] - bw_head[u] == 0)
      scc_id[u] = scc_id_singular;
}
