#pragma once

#include <ispan/util.hpp>

#include <set>

inline void
process_wcc(index_t vert_beg, index_t vert_end, vertex_t* wcc_fq, vertex_t const* color, vertex_t& wcc_fq_size)
{
  std::set<int> s_fq;
  for (vertex_t i = vert_beg; i < vert_end; ++i)
    if (not s_fq.contains(i))
      s_fq.insert(color[i]);

  int i = 0;
  for (auto it = s_fq.begin(); it != s_fq.end(); ++it, ++i)
    wcc_fq[i] = *it;

  wcc_fq_size = static_cast<vertex_t>(s_fq.size());
}
