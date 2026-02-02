#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/memory/accessor/once_queue_accessor.hpp>
#include <kaspan/scc/frontier.hpp>

namespace kaspan {

/// trim_1_exhaustive iteratevely trims one direction, either forward or backward, exhaustive.
/// To do that scc_id must be valid and degree must be valid if:
/// scc_id[k] != undecided || valid(degree[k]).
template<part_view_concept Part = single_part_view>
void
trim_1_exhaustive(
  graph_part_view<Part>   graph,
  u64*                    is_undecided_storage,
  vertex_t*               degree,
  frontier_view<vertex_t> frontier,
  auto&&                  on_decision)
{
  auto is_undecided = view_bits(is_undecided_storage, graph.part.local_n());

  do {
    while (frontier.has_next()) {

      // Each vertex that we get from the queue
      // has a neighbour that was decided. This
      // is the location where we update the degree
      // accordingly if this vertex is not alredy
      // decided itself!

      auto const u = frontier.next();
      auto const k = graph.part.to_local(u);

      if (is_undecided.get(k)) {
        if (--degree[k]; degree[k] == 0) { // decide and prepare to notify neighbours
          is_undecided.unset(k);
          on_decision(k, u);
          graph.each_v(k, [&](auto v) { frontier.relaxed_push(graph.part, v); });
        }
      }
    }
  } while (frontier.comm(graph.part));
}

template<part_view_concept Part>
void
trim_1_exhaustive(
  bidi_graph_part_view<Part> graph,
  u64*                       is_undecided_storage,
  vertex_t*                  outdegree,
  vertex_t*                  indegree,
  frontier_view<vertex_t>    frontier,
  vertex_t*                  decided_beg,
  vertex_t*                  decided_end,
  auto&&                     on_decision)
{
  for (auto it = decided_beg; it != decided_end; ++it) {
    graph.each_v(*it, [&](auto v) { frontier.relaxed_push(graph.part, v); });
  }
  trim_1_exhaustive(graph.fw_view(), is_undecided_storage, indegree, frontier, on_decision);

  for (auto it = decided_beg; it != decided_end; ++it) {
    graph.each_bw_v(*it, [&](auto v) { frontier.relaxed_push(graph.part, v); });
  }
  trim_1_exhaustive(graph.bw_view(), is_undecided_storage, outdegree, frontier, on_decision);
}

/// trim_1_exhaustive_first iteratively trims vertices with indegree/outdegree of zero.
/// It assumes to run on a fresh graph with uninitialised scc_id/indegree/outdegree and
/// will initilise these appropriately.
template<part_view_concept Part>
void
trim_1_exhaustive_first(
  bidi_graph_part_view<Part> g,
  u64*                       is_undecided_storage,
  vertex_t*                  outdegree,
  vertex_t*                  indegree,
  frontier_view<vertex_t>    frontier,
  auto&&                     on_decision)
{
  auto is_undecided = view_bits(is_undecided_storage, g.part.local_n());

  // == initial indegree trim ==
  // We get the initial indegree right from the head array
  // and decide vertices with indegree of zero right away.
  // For every non resolved vertex we need to store the right
  // indegree and mark as undecided. (As this function is
  // expected to initialise the scc_id array)
  g.each_k([&](auto k) {
    auto const degree = g.indegree(k);

    if (degree > 0) { // initialise
      indegree[k] = degree;
    }

    else { // decide right away and prepare to notify neighbours
      DEBUG_ASSERT(is_undecided.get(k));
      is_undecided.unset(k);
      on_decision(k, g.part.to_global(k));
      g.each_v(k, [&](auto v) { frontier.relaxed_push(g.part, v); });
    }
  });

  // With the initialisation done and the first collected decision we
  // can now simply delegate the exhaustive trimming to a sub routine.
  trim_1_exhaustive(g.fw_view(), is_undecided_storage, indegree, frontier, on_decision);

  // == initial outdegree trim ==
  // We do similar to the initial indegree trim
  // but scc_id is already initialised and we might
  // already have decided vertices to skip.
  is_undecided.each(g.part.local_n(), [&](auto k) {
    auto const degree = g.outdegree(k);

    if (degree > 0) { // initialise
      outdegree[k] = degree;
    }

    else { // decide right away and prepare to notify neighbours
      is_undecided.unset(k);
      on_decision(k, g.part.to_global(k));
      g.each_bw_v(k, [&](auto v) { frontier.relaxed_push(g.part, v); });
    }
  });

  // delegate the exhaustive trimming to a sub routine again.
  trim_1_exhaustive(g.bw_view(), is_undecided_storage, outdegree, frontier, on_decision);
}

} // namespace kaspan
