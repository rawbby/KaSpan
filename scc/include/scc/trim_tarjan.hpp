#pragma once

#include <scc/part.hpp>
#include <scc/tarjan.hpp>

#include <unordered_set>
#include <iostream>

template<WorldPartConcept Part>
vertex_t
trim_tarjan(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  KASPAN_STATISTIC_SCOPE("trim_tarjan");
  std::unordered_set<vertex_t> comp;

  vertex_t decided = 0;

  // use the tarjan algorithm to perform a reduce step on all local components
  // this is more powerful than trim-1/trim-2/trim-3 but might also be very expensive

  tarjan(part, fw_head, fw_csr, [=, &comp, &decided](vertex_t cn, vertex_t const* cs) -> void {
    DEBUG_ASSERT_GT(cn, 0);
    DEBUG_ASSERT_LT(cn, part.local_n());

    if (cn == 1) {
      auto const k = cs[0];

      auto const out_degree = fw_head[k + 1] - fw_head[k];
      auto const in_degree  = bw_head[k + 1] - bw_head[k];
      if (in_degree > 0 and out_degree > 0) {
        return;
      }
      scc_id[k] = part.to_global(k);
      ++decided;
      return;
    }

    auto out_degree = 0;
    auto in_degree  = 0;
    auto min_v      = std::numeric_limits<vertex_t>::max();

    // reuse the set storage to reduce allocations
    comp.clear();
    comp.reserve(cn * 2);

    for (vertex_t c = 0; c < cn; ++c) {
      auto const k = cs[c];
      auto const v = part.to_global(k);

      min_v = std::min(min_v, v);
      comp.emplace(v);
    }

    for (vertex_t c = 0; c < cn; ++c) {
      auto const k = cs[c];
      auto const v = part.to_global(k);

      // forward
      {
        auto const beg = fw_head[k];
        auto const end = fw_head[k + 1];
        for (auto it = beg; it < end; ++it) {
          auto const u = fw_csr[it];
          if (not comp.contains(u)) {
            ++out_degree;
            if (in_degree > 0 and out_degree > 0) {
              return;
            }
          }
        }
      }

      // backward
      {
        auto const beg = bw_head[k];
        auto const end = bw_head[k + 1];
        for (auto it = beg; it < end; ++it) {
          auto const u = bw_csr[it];
          if (not comp.contains(u)) {
            ++in_degree;
            if (in_degree > 0 and out_degree > 0) {
              return;
            }
          }
        }
      }
    }

    // found a global component (no external edges in one direction)
    for (vertex_t c = 0; c < cn; ++c) {
      auto const k = cs[c];
      scc_id[k]    = min_v;
    }
    decided += cn;
  });

  KASPAN_STATISTIC_ADD("decision_count", decided);
  return decided;
}
