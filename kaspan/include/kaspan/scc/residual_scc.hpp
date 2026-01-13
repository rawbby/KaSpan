#pragma once

#include <kaspan/debug/statistic.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/graph.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <unordered_set>

namespace kaspan {

inline void
residual_scc(
  vertex_t const* wcc_id,
  vertex_t        wcc_count,
  vertex_t*       sub_scc_id,
  vertex_t        n,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  vertex_t const* sub_ids_inverse)
{
  KASPAN_STATISTIC_SCOPE("residual_scc");

  // prepare the local partition this rank calculates
  auto const part = balanced_slice_part{ wcc_count };

  std::vector<vertex_t>        queue;
  std::unordered_set<vertex_t> fw_reach;

  size_t decision_count = 0;

  for (size_t root = 0; root < n; ++root) {
    if (sub_scc_id[root] == scc_id_undecided and part.has_local(wcc_id[root])) {

      auto const id = sub_ids_inverse[root]; // directly use the global id so later no translation is needed

      sub_scc_id[root] = id;
      ++decision_count;

      // fw search
      fw_reach.emplace(root);
      queue.emplace_back(root);
      while (not queue.empty()) {
        auto const u = queue.back();
        queue.pop_back();

        auto const beg = fw_head[u];
        auto const end = fw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = fw_csr[it];
          if (sub_scc_id[v] == scc_id_undecided and fw_reach.emplace(v).second) {
            queue.emplace_back(v);
          }
        }
      }

      // bw search
      queue.emplace_back(root);
      while (not queue.empty()) {
        auto const u = queue.back();
        queue.pop_back();

        auto const beg = bw_head[u];
        auto const end = bw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = bw_csr[it];
          if (sub_scc_id[v] == scc_id_undecided and fw_reach.contains(v)) {
            queue.emplace_back(v);
            sub_scc_id[v] = id;
            ++decision_count;
          }
        }
      }

      fw_reach.clear();
    }
  }

  KASPAN_STATISTIC_ADD("decision_count", decision_count);
}

} // namespace kaspan
