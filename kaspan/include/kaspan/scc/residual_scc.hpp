#pragma once

#include <kaspan/debug/statistic.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <unordered_set>

namespace kaspan {

template<world_part_concept Part>
inline void
residual_scc(
  vertex_t const*             wcc_id,
  vertex_t*                   sub_scc_id,
  bidi_graph_part_view<Part>  graph,
  vertex_t const*             sub_ids_inverse)
{
  KASPAN_STATISTIC_SCOPE("residual_scc");

  auto const& part    = *graph.part;
  auto const  local_n = part.local_n();

  std::vector<vertex_t>        queue;
  std::unordered_set<vertex_t> fw_reach;

  size_t decision_count = 0;

  for (vertex_t root = 0; root < local_n; ++root) {
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

        for (auto const v_global : graph.fw_view().csr_range(u)) {
          if (part.has_local(v_global)) {
            auto const v = part.to_local(v_global);
            if (sub_scc_id[v] == scc_id_undecided and fw_reach.emplace(v).second) {
              queue.emplace_back(v);
            }
          }
        }
      }

      // bw search
      queue.emplace_back(root);
      while (not queue.empty()) {
        auto const u = queue.back();
        queue.pop_back();

        for (auto const v_global : graph.bw_view().csr_range(u)) {
          if (part.has_local(v_global)) {
            auto const v = part.to_local(v_global);
            if (sub_scc_id[v] == scc_id_undecided and fw_reach.contains(v)) {
              queue.emplace_back(v);
              sub_scc_id[v] = id;
              ++decision_count;
            }
          }
        }
      }

      fw_reach.clear();
    }
  }

  KASPAN_STATISTIC_ADD("decision_count", decision_count);
}

} // namespace kaspan
