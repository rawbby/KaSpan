#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/concept.hpp>

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

template<part_view_concept Part>
void
normalise_scc_id(
  Part      part,
  vertex_t* scc_id)
{
  auto old2new = std::unordered_map<vertex_t, vertex_t>{};
  old2new.reserve(static_cast<size_t>(part.local_n()));

  auto [scc_id_full, n] = mpi_basic::allgatherv(scc_id, part.local_n());
  ASSERT_EQ(n, part.n(), "all local n should sum up to n");

  for (vertex_t u = 0; u < n; ++u) {
    if (!old2new.contains(scc_id_full[u])) {
      old2new[scc_id_full[u]] = u;
    }
    if (part.has_local(u)) {
      auto const k = part.to_local(u);
      scc_id[k]    = old2new[scc_id_full[u]];
    }
  }
}

} // namespace kaspan
