#pragma once

#include <graph/Graph.hpp>
#include <scc/Common.hpp>
#include <util/ScopeGuard.hpp>

#include <kamping/communicator.hpp>

#include <unordered_set>

inline void
FwBwPart(kamping::Communicator<>& comm, U64Buffer const& wcc_id, size_t wcc_count, U64Buffer& sub_scc_id, Graph const& sub_graph, U64Buffer const& sub_ids_inverse)
{
  // prepare the local partition this rank calculates
  auto const part = BalancedSlicePart{ wcc_count, comm.rank(), comm.size() };

  std::vector<vertex_t>        queue;
  std::unordered_set<vertex_t> fw_reach;

  for (size_t root = 0; root < sub_graph.n; ++root) {
    if (sub_scc_id[root] == scc_id_undecided and part.contains(wcc_id[root])) {

      auto const id = sub_ids_inverse[root]; // directly use the global id so later no translation is needed

      sub_scc_id[root] = id;

      // fw search
      fw_reach.emplace(root);
      queue.emplace_back(root);
      while (not queue.empty()) {
        auto const u = queue.back();
        queue.pop_back();

        auto const beg = sub_graph.fw_head[u];
        auto const end = sub_graph.fw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = sub_graph.fw_csr[it];
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

        auto const beg = sub_graph.bw_head[u];
        auto const end = sub_graph.bw_head[u + 1];
        for (auto it = beg; it < end; ++it) {
          auto const v = sub_graph.bw_csr[it];
          if (sub_scc_id[v] == scc_id_undecided and fw_reach.contains(v)) {
            queue.emplace_back(v);
            sub_scc_id[v] = id;
          }
        }
      }

      fw_reach.clear();
    }
  }
}
