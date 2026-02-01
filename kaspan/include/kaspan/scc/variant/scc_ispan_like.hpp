#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>

#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
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
  vertex_t local_decided       = 0;
  vertex_t global_decided      = 0;
  vertex_t prev_local_decided  = 0;
  vertex_t prev_global_decided = 0;

  auto const on_decision = [&](auto k, auto id) {
    scc_id[k] = id;
    ++local_decided;
  };

  KASPAN_STATISTIC_PUSH("trim_1_first");
  auto is_undecided = make_bits_filled(graph.part.local_n());
  auto pivot        = trim_1_first(graph, is_undecided.data(), on_decision);
  global_decided    = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("pivot", pivot);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  auto front          = frontier{ graph.part.local_n() };
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto active         = make_array<vertex_t>(graph.part.local_n());
  auto is_reached     = make_bits(graph.part.local_n());
  forward_backward_search(graph, front.view<vertex_t>(), active.data(), is_reached.data(), is_undecided.data(), pivot, on_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  trim_1_normal(graph, is_undecided.data(), on_decision);
  trim_1_normal(graph, is_undecided.data(), on_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("residual");
  auto [sub_ids_inverse, sub_g] =
    allgather_fw_sub_graph(graph.part, graph.part.local_n() - local_decided, graph.fw.head, graph.fw.csr, [&](auto k) { return is_undecided.get(k); });
  tarjan(sub_g.view(), [&](auto const* beg, auto const* end) {
    auto const id = sub_ids_inverse[*std::min_element(beg, end)];
    for (auto sub_u : std::span{ beg, end }) {
      auto const u = sub_ids_inverse[sub_u];
      if (graph.part.has_local(u)) {
        scc_id[graph.part.to_local(u)] = id;
      }
    }
  });
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", graph.part.local_n() - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", graph.part.n() - prev_global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
