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

namespace kaspan {

template<part_view_concept Part>
void
scc_trim_ex_light_residual(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  KASPAN_STATISTIC_SCOPE("scc");

  auto front     = frontier{ graph.part.local_n() };
  auto message_buffer = vector<vertex_t>{};
  auto outdegree = make_array<vertex_t>(graph.part.local_n());
  auto indegree  = make_array<vertex_t>(graph.part.local_n());

  auto const undecided_filter = SCC_ID_UNDECIDED_FILTER(graph.part.local_n(), scc_id);

  auto       decided_stack = make_stack<vertex_t>(graph.part.local_n());
  auto const on_decision   = [&](vertex_t k) { decided_stack.push(k); };

  KASPAN_STATISTIC_PUSH("trim_ex_first");
  vertex_t local_decided  = trim_1_exhaustive_first(graph, scc_id, outdegree.data(), indegree.data(), front.view<vertex_t>());
  vertex_t global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  auto const decided_threshold   = graph.part.n() - (graph.part.n() / mpi_basic::world_size);
  vertex_t   prev_local_decided  = local_decided;
  vertex_t   prev_global_decided = global_decided;

  if (global_decided < decided_threshold) {
    KASPAN_STATISTIC_PUSH("forward_backward_search");
    vertex_t prev_local_decided  = local_decided;
    vertex_t prev_global_decided = global_decided;
    auto active       = make_array<vertex_t>(graph.part.local_n());
    auto is_reached   = make_bits(graph.part.local_n());
    auto is_undecided = make_bits(graph.part.local_n());
    {
      is_undecided.set_each(graph.part.local_n(), [&](auto k) { return scc_id[k] == scc_id_undecided; });
      auto const pivot = select_pivot_from_degree(graph.part, scc_id, outdegree.data(), indegree.data());
      forward_backward_search(graph, front.view<vertex_t>(), active.data(), is_reached.data(), is_undecided.data(), pivot, [&](auto k, auto id) {
        scc_id[k] = id;
        ++local_decided;
      });
    }
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
    KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
    KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    KASPAN_STATISTIC_POP();

    if (global_decided < decided_threshold) {
      KASPAN_STATISTIC_PUSH("color");

      auto colors       = make_array<vertex_t>(graph.part.local_n());
      auto active_array = make_array<vertex_t>(graph.part.local_n() - local_decided);

      vertex_t iterations = 0;
      prev_local_decided  = local_decided;
      prev_global_decided = global_decided;

      auto label        = make_array<vertex_t>(graph.part.local_n());
      auto has_changed  = make_bits_clean(graph.part.local_n());
      is_undecided.set_each(graph.part.local_n(), [&](auto k) { return scc_id[k] == scc_id_undecided; });
      do {
        label_search(graph, front.view<edge_t>(), colors.data(), active.data(), is_reached.data(), has_changed.data(), is_undecided.data(), [&](auto k, auto id) {
          scc_id[k] = id;
          ++local_decided;
          decided_stack.push(k);
        });
        global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
        ++iterations;

      } while (global_decided < decided_threshold);

      KASPAN_STATISTIC_ADD("iterations", iterations);
      KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
      KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
      KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
      KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
      KASPAN_STATISTIC_POP();
    }
  }

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_ex");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  local_decided += trim_1_exhaustive(graph, scc_id, outdegree.data(), indegree.data(), front.view<vertex_t>(), decided_stack.begin(), decided_stack.end());
  decided_stack.clear();
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("local_decided", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("global_decided", global_decided - prev_global_decided);
  KASPAN_STATISTIC_ADD("decided_count", global_decided - prev_global_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("residual");
  auto [sub_ids_inverse, sub_g] = allgather_fw_sub_graph(graph.part, graph.part.local_n() - local_decided, graph.fw.head, graph.fw.csr, undecided_filter);
  tarjan(sub_g.view(), [&](auto const* beg, auto const* end) {
    auto const id = sub_ids_inverse[*std::min_element(beg, end)];
    for (auto sub_u : std::span{ beg, end }) {
      auto const u = sub_ids_inverse[sub_u];
      if (graph.part.has_local(u)) {
        scc_id[graph.part.to_local(u)] = id;
      }
    }
  });
  KASPAN_STATISTIC_ADD("n", sub_g.n);
  KASPAN_STATISTIC_ADD("m", sub_g.m);
  KASPAN_STATISTIC_ADD("local_decided", graph.part.local_n() - local_decided);
  KASPAN_STATISTIC_ADD("global_decided", graph.part.n() - global_decided);
  KASPAN_STATISTIC_ADD("decided_count", graph.part.n() - global_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
