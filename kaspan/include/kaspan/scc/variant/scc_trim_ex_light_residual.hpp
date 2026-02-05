#pragma once

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>
#include <kaspan/scc/pivot.hpp>
#include <kaspan/scc/tarjan.hpp>
#include <kaspan/scc/trim_1_exhaustive.hpp>
#include <kaspan/scc/multi_pivot_search.hpp>
#include <kaspan/scc/pivot_search.hpp>

namespace kaspan {

template<part_view_c Part>
void
scc_trim_ex_light_residual(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");
  vertex_t local_decided       = 0;
  vertex_t global_decided      = 0;
  vertex_t prev_local_decided  = 0;
  vertex_t prev_global_decided = 0;

  auto const on_trim_decision = [&](auto k, auto id) {
    scc_id[k] = id;
    ++local_decided;
  };

  KASPAN_STATISTIC_PUSH("trim_ex_first");
  auto front        = frontier{ graph.part.local_n() };
  auto outdegree    = make_array<vertex_t>(graph.part.local_n());
  auto indegree     = make_array<vertex_t>(graph.part.local_n());
  auto is_undecided = make_bits_filled(graph.part.local_n());
  trim_1_exhaustive_first(graph, is_undecided.data(), outdegree.data(), indegree.data(), front.view<vertex_t>(), on_trim_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  auto const decided_threshold = graph.part.n() - (graph.part.n() / mpi_basic::world_size);

  auto       decided_stack = make_stack<vertex_t>(graph.part.local_n());
  auto const on_decision   = [&](auto k, auto id) {
    on_trim_decision(k, id);
    decided_stack.push(k);
  };

  if (global_decided < decided_threshold) {
    KASPAN_STATISTIC_PUSH("forward_backward_search");
    prev_local_decided    = local_decided;
    prev_global_decided   = global_decided;
    auto       active     = make_array<vertex_t>(graph.part.local_n());
    auto       is_reached = make_bits(graph.part.local_n());
    auto const pivot      = select_pivot_from_degree(graph.part, is_undecided.data(), outdegree.data(), indegree.data());
    forward_backward_search(graph, front.view<vertex_t>(), active.data(), is_reached.data(), is_undecided.data(), pivot, on_decision);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    KASPAN_STATISTIC_POP();

    if (global_decided < decided_threshold) {
      KASPAN_STATISTIC_PUSH("color");

      auto colors       = make_array<vertex_t>(graph.part.local_n());
      auto active_array = make_array<vertex_t>(graph.part.local_n() - local_decided);

      prev_local_decided  = local_decided;
      prev_global_decided = global_decided;

      auto label       = make_array<vertex_t>(graph.part.local_n());
      auto has_changed = make_bits_clean(graph.part.local_n());
      do {
        rot_label_search(graph, front.view<edge_t>(), colors.data(), active.data(), is_reached.data(), has_changed.data(), is_undecided.data(), on_decision);
        global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);

      } while (global_decided < decided_threshold);

      KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
      KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
      KASPAN_STATISTIC_POP();
    }
  }

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_ex");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  trim_1_exhaustive(graph, is_undecided.data(), outdegree.data(), indegree.data(), front.view<vertex_t>(), decided_stack.begin(), decided_stack.end(), on_trim_decision);
  decided_stack.clear();
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
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
  KASPAN_STATISTIC_ADD("decided_count", graph.part.local_n() - local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
