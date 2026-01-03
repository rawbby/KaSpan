#pragma once

#include <ispan/util.hpp>

#include <map>
#include <unordered_map>
#include <unordered_set>

inline void
get_scc_result(kaspan::vertex_t* scc_id, kaspan::vertex_t n)
{
  kaspan::vertex_t singular_count = 0;
  kaspan::vertex_t largest_size   = 0;

  std::map<kaspan::vertex_t, kaspan::vertex_t> component_sizes;
  for (kaspan::vertex_t i = 0; i < n; ++i) {
    if (scc_id[i] == scc_id_largest) largest_size++;
    else if (scc_id[i] == kaspan::scc_id_singular) singular_count++;
    else component_sizes[scc_id[i]]++;
  }

  auto const normal_count = component_sizes.size();

  // map ispan scc id to normalized scc id
  std::unordered_map<kaspan::vertex_t, kaspan::vertex_t> cid;
  cid.reserve(1 + normal_count);

  for (kaspan::vertex_t i = 0; i < n; ++i) {
    if (scc_id[i] == kaspan::scc_id_singular) scc_id[i] = i;

    else {
      auto [it, inserted] = cid.try_emplace(scc_id[i], i);
      if (inserted) scc_id[i] = i;
      else // take present id
        scc_id[i] = it->second;
    }
  }
}
