#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>

#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/backward_search.hpp>
#include <kaspan/scc/forward_search.hpp>
#include <kaspan/scc/tarjan.hpp>
#include <kaspan/scc/trim_1_local.hpp>

namespace kaspan {

template<part_view_concept Part>
void
scc_ispan_like(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");

  KASPAN_STATISTIC_PUSH("trim_1_first");
  auto [local_decided, pivot] = trim_1_first(graph, scc_id);
  auto global_decided         = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("pivot", pivot);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  vertex_t prev_local_decided  = local_decided;
  vertex_t prev_global_decided = global_decided;
  {
    auto front          = frontier{ graph.part.local_n() };
    auto message_buffer = vector<vertex_t>{};
    auto reached = make_bits_clean(graph.part.local_n());
    forward_search(graph, front.view<vertex_t>(), message_buffer, scc_id, reached.data(), pivot);
    local_decided += backward_search(graph, front.view<vertex_t>(), message_buffer, scc_id, reached.data(), pivot);
  }
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  local_decided += trim_1_normal(graph, scc_id);
  local_decided += trim_1_normal(graph, scc_id);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("residual");
  prev_local_decided            = local_decided;
  prev_global_decided           = global_decided;
  auto const undecided_filter   = SCC_ID_UNDECIDED_FILTER(graph.part.local_n(), scc_id);
  auto [sub_ids_inverse, sub_g] = allgather_fw_sub_graph(graph.part, graph.part.local_n() - local_decided, graph.fw.head, graph.fw.csr, undecided_filter);
  tarjan(sub_g.view(), [&](auto const* beg, auto const* end) {
    auto const id = sub_ids_inverse[*std::min_element(beg, end)];
    for (auto sub_u : std::span{ beg, end }) {
      auto const u = sub_ids_inverse[sub_u];
      if (graph.part.has_local(u)) {
        scc_id[graph.part.to_local(u)] = id;
        ++local_decided;
      }
    }
  });
  KASPAN_STATISTIC_ADD("n", sub_g.n);
  KASPAN_STATISTIC_ADD("m", sub_g.m);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
