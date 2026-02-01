#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/scc/frontier.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

template<part_view_concept Part>
auto
trim_1_first(
  bidi_graph_part_view<Part> g,
  u64*                       is_undecided_storage,
  auto&&                     on_decision) -> vertex_t
{
  auto is_undecided    = view_bits(is_undecided_storage, g.part.local_n());
  u64  max_degree_prod = 0;

  index_t pivot = g.part.n();

  for (vertex_t k = 0; k < g.part.local_n(); ++k) {
    auto const outdegree = g.outdegree(k);
    auto const indegree  = g.indegree(k);

    if (indegree == 0 || outdegree == 0) {
      is_undecided.unset(k);
      on_decision(k, g.part.to_global(k));
    } else {
      auto const degree_prod = integral_cast<u64>(indegree) * outdegree;
      if (max_degree_prod < degree_prod) {
        max_degree_prod = degree_prod;
        pivot           = g.part.to_global(k);
      }
    }
  }

  if (max_degree_prod != mpi_basic::allreduce_single(max_degree_prod, mpi_basic::max)) {
    // overwrite as not global max degree
    pivot = g.part.n();
  }

  return mpi_basic::allreduce_single(pivot, mpi_basic::min);
}

template<part_view_concept Part = single_part_view>
void
trim_1_normal(
  bidi_graph_part_view<Part> graph,
  u64*                       is_undecided_storage,
  auto&&                     on_decision)
{
  auto is_undecided = view_bits(is_undecided_storage, graph.part.local_n());

  auto const has_degree = [&](bidi_graph_part_view<Part> g, auto k) {
    for (auto v : g.csr_range(k)) {
      if (!g.part.has_local(v) || is_undecided.get(g.part.to_local(v))) return true;
    }
    for (auto v : g.bw_csr_range(k)) {
      if (!g.part.has_local(v) || is_undecided.get(g.part.to_local(v))) return true;
    }
    return false;
  };

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    if (is_undecided.get(k) && !has_degree(graph, k)) {
      is_undecided.unset(k);
      on_decision(k, graph.part.to_global(k));
    }
  }
}

} // namespace kaspan
