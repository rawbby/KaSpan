#pragma once

#include <ispan/util.hpp>

#include <cassert>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <unordered_set>

inline void
get_scc_result(index_t* scc_id, index_t vert_count)
{
  index_t size_1  = 0;
  index_t largest = 0;

  std::map<index_t, index_t> mp;
  for (index_t i = 0; i < vert_count + 1; ++i) {
    if (scc_id[i] == 1)
      largest++;
    else if (scc_id[i] == -1)
      size_1++;
    else
      mp[scc_id[i]]++;
  }

  printf("\nResult:\nlargest, %d\ntrimmed size_1, %d\ntrimmed size_2, %lu\ntotal, %lu\n", largest, size_1, mp.size(), 1 + size_1 + mp.size());

  // bring the scc id into a normalized version

  std::unordered_map<index_t, index_t> min_node_in_component;
  min_node_in_component.reserve(vert_count);

  for (index_t i = 0; i < vert_count; ++i) {
    if (scc_id[i] == -1) {
      scc_id[i] = i;
    } else {
      auto const id       = scc_id[i];
      auto [it, inserted] = min_node_in_component.try_emplace(id, i);
      if (not inserted)
        scc_id[i] = it->second;
    }
  }
}
