#pragma once

#include "kaspan/memory/hash_set.hpp"

#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/bits.hpp>
#include <kaspan/memory/hash_map.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>
#include <kaspan/scc/cache_multi_pivot_search.hpp>
#include <kaspan/scc/cache_pivot_search.hpp>
#include <kaspan/scc/trim_1_local.hpp>

namespace kaspan {

template<part_view_c Part = single_part_view>
void
scc(
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
  auto       is_undecided = make_bits_filled(graph.part.local_n());
  auto const pivot        = trim_1_first(graph, is_undecided.data(), on_decision);
  global_decided          = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto front          = frontier{ graph.part.local_n() };
  auto active         = make_array<vertex_t>(graph.part.local_n());
  auto is_reached     = make_bits(graph.part.local_n());
  auto reached_cache  = hash_set<vertex_t>{ graph.local_fw_m + graph.local_bw_m };
  cache_forward_backward_search(graph, front, active.data(), is_reached.data(), is_undecided.data(), pivot, on_decision, reached_cache);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  trim_1_normal(graph, is_undecided.data(), on_decision);
  global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
  KASPAN_STATISTIC_POP();

  if (global_decided == graph.part.n()) return;

  KASPAN_STATISTIC_PUSH("color");
  prev_local_decided  = local_decided;
  prev_global_decided = global_decided;
  auto label          = make_array<vertex_t>(graph.part.local_n());
  auto has_changed    = make_bits_clean(graph.part.local_n());
  auto label_cache    = hash_map<vertex_t>{ graph.local_fw_m + graph.local_bw_m };
  for (;;) {
    cache_rot_label_search(graph, front, label.data(), active.data(), is_reached.data(), has_changed.data(), is_undecided.data(), on_decision, label_cache);
    global_decided = mpi_basic::allreduce_single(local_decided, mpi_basic::sum);
    if (global_decided >= graph.part.n()) break;
    label_cache.clear();
  }
  KASPAN_STATISTIC_ADD("decided_count", local_decided - prev_local_decided);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_POP();
}

} // namespace kaspan
