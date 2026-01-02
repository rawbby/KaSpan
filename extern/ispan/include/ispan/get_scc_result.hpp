#pragma once

#include <ispan/util.hpp>

#include <map>
#include <unordered_map>
#include <unordered_set>

inline void
get_scc_result(vertex_t* scc_id, vertex_t n)
{
  vertex_t singular_count = 0;
  vertex_t largest_size   = 0;

  std::map<vertex_t, vertex_t> component_sizes;
  for (vertex_t i = 0; i < n; ++i) {
    if (scc_id[i] == scc_id_largest) largest_size++;
    else if (scc_id[i] == scc_id_singular) singular_count++;
    else component_sizes[scc_id[i]]++;
  }

  auto const normal_count = component_sizes.size();

  // map ispan scc id to normalized scc id
  std::unordered_map<vertex_t, vertex_t> cid;
  cid.reserve(1 + normal_count);

  for (vertex_t i = 0; i < n; ++i) {
    if (scc_id[i] == scc_id_singular) scc_id[i] = i;

    else {
      auto [it, inserted] = cid.try_emplace(scc_id[i], i);
      if (inserted) scc_id[i] = i;
      else // take present id
        scc_id[i] = it->second;
    }
  }
}
