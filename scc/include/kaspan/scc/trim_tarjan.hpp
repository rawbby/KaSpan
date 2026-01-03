#pragma once

#include <kaspan/scc/part.hpp>
#include <kaspan/scc/tarjan.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <unordered_set>

namespace kaspan {

namespace internal::trim_tarjan {

using contains_fn = bool (*)(std::span<vertex_t>, vertex_t const&);

inline bool
linear_contains(std::span<vertex_t> span, vertex_t const& value)
{
  return std::ranges::find(span, value) != span.end();
}

inline bool
binary_contains(std::span<vertex_t> span, vertex_t const& value)
{
  return std::ranges::binary_search(span, value);
}

inline auto
choose_contains_fn(std::span<vertex_t> span)
{
  DEBUG_ASSERT_GT(span.size(), 0);

  struct root_and_fn
  {
    vertex_t    min;
    contains_fn contains;
  };

  if (span.size_bytes() >= 256) [[unlikely]] {
    std::ranges::sort(span);
    return root_and_fn{ span[0], binary_contains };
  }

  return root_and_fn{ *std::ranges::min_element(span), linear_contains };
}

}

template<world_part_concept part_t, typename filter_t = decltype(no_filter)>
vertex_t
trim_tarjan(part_t const&   part,
            index_t const*  fw_head,
            vertex_t const* fw_csr,
            index_t const*  bw_head,
            vertex_t const* bw_csr,
            vertex_t*       scc_id,
            filter_t        filter        = no_filter,
            vertex_t        decided_count = 0)
{
  using namespace internal::trim_tarjan;
  KASPAN_STATISTIC_SCOPE("trim_tarjan");

  vertex_t decided = 0;

  // use the tarjan algorithm to perform a reduce step on all local components
  // this is more powerful than trim-1/trim-2/trim-3 but might also be very expensive

  tarjan(
    part,
    fw_head,
    fw_csr,
    [=, &decided, &filter](vertex_t* cbeg, vertex_t* cend) -> void {
      auto const component = std::span{ cbeg, cend };

      DEBUG_ASSERT_GT(component.size(), 0);
      DEBUG_ASSERT_LE(component.size(), part.local_n());
      [[maybe_unused]] auto const clen = static_cast<vertex_t>(component.size());

      if (clen == 1) {
        auto const k = component[0];

        auto const out_degree = fw_head[k + 1] - fw_head[k];
        auto const in_degree  = bw_head[k + 1] - bw_head[k];
        if (in_degree > 0 and out_degree > 0) { return; }
        scc_id[k] = part.to_global(k);
        ++decided;
        return;
      }

      index_t out_degree          = 0;
      index_t in_degree           = 0;
      auto const [root, contains] = choose_contains_fn(component);

      for (auto const k : component) {
        DEBUG_ASSERT(filter(k));
        auto const v = part.to_global(k);

        // forward
        {
          auto const beg = fw_head[k];
          auto const end = fw_head[k + 1];
          for (auto it = beg; it < end; ++it) {
            auto const u = fw_csr[it];
            if (not contains(component, u) and (not part.has_local(u) or filter(part.to_local(u)))) {
              ++out_degree;
              if (in_degree > 0 and out_degree > 0) { return; }
            }
          }
        }

        // backward
        {
          auto const beg = bw_head[k];
          auto const end = bw_head[k + 1];
          for (auto it = beg; it < end; ++it) {
            auto const u = bw_csr[it];
            if (not contains(component, u) and (not part.has_local(u) or filter(part.to_local(u)))) {
              ++in_degree;
              if (in_degree > 0 and out_degree > 0) { return; }
            }
          }
        }
      }

      // found a global component (no external edges in one direction)
      for (auto const k : component) { scc_id[k] = root; }

      decided += clen;
    },
    filter,
    decided_count);

  KASPAN_STATISTIC_ADD("decision_count", decided);
  return decided;
}

} // namespace kaspan
