#pragma once

#include "kaspan/debug/process.hpp"
#include "kaspan/scc/allgather_sub_graph.hpp"

#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/backward_search.hpp>
#include <kaspan/scc/forward_search.hpp>
#include <kaspan/scc/trim_1_local.hpp>

namespace kaspan {

template<part_view_concept Part>
void
scc_ispan_like(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  KASPAN_STATISTIC_PUSH("trim_1_first");
  auto [local_decided, pivot] = trim_1_first(graph, scc_id);
  auto global_decided         = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("pivot", pivot);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_POP();

  if (pivot == graph.part.n()) return;

  auto front      = frontier{ graph.part.local_n() };
  auto bitbuffer0 = make_bits_clean(graph.part.local_n());
  auto bitbuffer1 = make_bits_clean(graph.part.local_n());

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  forward_search(graph, front.view<vertex_t>(), scc_id, bitbuffer0.data(), bitbuffer1.data(), pivot, local_decided);
  auto const fwbw_local_decided  = backward_search(graph, front.view<vertex_t>(), scc_id, bitbuffer0.data(), bitbuffer1.data(), pivot, local_decided, [](auto) {});
  auto const fwbw_global_decided = mpi_basic::allreduce_single(fwbw_local_decided, mpi_basic::sum);
  local_decided += fwbw_local_decided;
  global_decided += fwbw_global_decided;
  KASPAN_STATISTIC_ADD("local_decided", fwbw_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", fwbw_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  auto const trim_1_local_decided  = trim_1_normal(graph, scc_id) + trim_1_normal(graph, scc_id);
  auto const trim_1_global_decided = mpi_basic::allreduce_single(trim_1_local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", trim_1_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", trim_1_global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("residual");
  auto const undecided_filter   = SCC_ID_UNDECIDED_FILTER(graph.part.local_n(), scc_id);
  auto [sub_ids_inverse, sub_g] = allgather_fw_sub_graph(graph.part, graph.part.local_n() - local_decided, graph.fw.head, graph.fw.csr, undecided_filter);
  if (sub_g.n) {
    tarjan(sub_g.view(), [&](auto const* beg, auto const* end) {
      auto const id = sub_ids_inverse[*std::min_element(beg, end)];
      for (auto sub_u : std::span{ beg, end }) {
        auto const u = sub_ids_inverse[sub_u];
        if (graph.part.has_local(u)) {
          scc_id[graph.part.to_local(u)] = id;
        }
      }
    });
  }
  KASPAN_STATISTIC_ADD("n", sub_g.n);
  KASPAN_STATISTIC_ADD("m", sub_g.m);
  KASPAN_STATISTIC_ADD("local_decided", graph.part.local_n() - local_decided);
  KASPAN_STATISTIC_ADD("global_decided", graph.part.n() - global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
