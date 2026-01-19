#pragma once

#include <kaspan/graph/base.hpp>

#include <unordered_map>

namespace kaspan {

inline void
normalise_scc_id(
  vertex_t  n,
  vertex_t* scc_id)
{
  auto old2new = std::unordered_map<vertex_t, vertex_t>{};
  old2new.reserve(static_cast<size_t>(n));

  for (vertex_t u = 0; u < n; ++u) {
    if (!old2new.contains(scc_id[u])) {
      old2new[scc_id[u]] = u;
    }
    scc_id[u] = old2new[scc_id[u]];
  }
}

} // namespace kaspan
