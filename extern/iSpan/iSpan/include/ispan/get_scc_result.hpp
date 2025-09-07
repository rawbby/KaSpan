#pragma once

#include <ispan/util.hpp>

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
  for (index_t i = 0; i < vert_count; ++i) {
    if (scc_id[i] == 1)
      largest++;
    else if (scc_id[i] == -1)
      size_1++;
    else
      mp[scc_id[i]]++;
  }

  printf("\nResult:\nlargest, %d\ntrimmed size_1, %d\ntrimmed size_2, %lu\ntotal, %lu\n", largest, size_1, mp.size(), 1 + size_1 + mp.size());

  // bring the scc id into a normalized version

  // map ispan scc id to normalized scc id
  std::unordered_map<index_t, index_t> cid;
  cid.reserve(vert_count);

  for (index_t i = 0; i < vert_count; ++i) {

    if (scc_id[i] != -1) {
      auto [it, inserted] = cid.try_emplace(scc_id[i], i);
      if (inserted)
        scc_id[i] = i;
      else // take present id
        scc_id[i] = it->second;
    }

    else // singular component
      scc_id[i] = i;
  }
}
