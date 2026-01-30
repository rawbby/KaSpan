#pragma once

#include <kaspan/scc/frontier.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/once_queue_accessor.hpp>

namespace kaspan {

/// trim_1_exhaustive iteratevely trims one direction, either forward or backward, exhaustive.
/// To do that scc_id must be valid and degree must be valid if:
/// scc_id[k] != undecided || valid(degree[k]).
template<u64               ThresholdBytes,
         part_view_concept Part>
auto
trim_1_exhaustive(
  graph_part_view<Part>             graph,
  vertex_t*                         scc_id,
  vertex_t*                         degree,
  frontier_view<vertex_t,
                ThresholdBytes> frontier) -> vertex_t
{
  auto     part          = graph.part;
  vertex_t decided_count = 0;

  do {
    while (frontier.has_next()) {

      // Each vertex that we get from the queue
      // has a neighbour that was decided. This
      // is the location where we update the degree
      // accordingly if this vertex is not alredy
      // decided itself!

      auto const u = frontier.next();
      auto const k = part.to_local(u);

      if (scc_id[k] == scc_id_undecided) {
        if (--degree[k]; degree[k] == 0) { // decide and prepare to notify neighbours
          scc_id[k] = u;
          ++decided_count;
          for (auto v : graph.csr_range(k)) {
            frontier.relaxed_push(part, v);
          }
        }
      }
    }
  } while (frontier.comm(part));

  return decided_count;
}

template<u64               CommThresholdBytes,
         part_view_concept Part>
auto
trim_1_exhaustive(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id,
  vertex_t*                  outdegree,
  vertex_t*                  indegree,
  frontier_view<vertex_t,
                CommThresholdBytes> frontier,
  vertex_t*                  decided_beg,
  vertex_t*                  decided_end) -> vertex_t
{
  auto     part          = graph.part;
  vertex_t decided_count = 0;

  // from all decided
  for (auto it = decided_beg; it != decided_end; ++it) {
    for (auto const v : graph.fw_view().csr_range(*it)) {
      frontier.relaxed_push(part, v);
    }
  }

  decided_count += trim_1_exhaustive(graph.fw_view(), scc_id, indegree, frontier);

  for (auto it = decided_beg; it != decided_end; ++it) {
    for (auto const v : graph.bw_view().csr_range(*it)) {
      frontier.relaxed_push(part, v);
    }
  }

  return decided_count + trim_1_exhaustive(graph.bw_view(), scc_id, outdegree, frontier);
}

/// trim_1_exhaustive_first iteratively trims vertices with indegree/outdegree of zero.
/// It assumes to run on a fresh graph with uninitialised scc_id/indegree/outdegree and
/// will initilise these appropriately.
template<u64               CommThresholdBytes,
         part_view_concept Part>
auto
trim_1_exhaustive_first(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id,
  vertex_t*                  outdegree,
  vertex_t*                  indegree,
  frontier_view<vertex_t,
                CommThresholdBytes> frontier) -> vertex_t
{
  auto       part          = graph.part;
  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;

  // == initial indegree trim ==
  // We get the initial indegree right from the head array
  // and decide vertices with indegree of zero right away.
  // For every non resolved vertex we need to store the right
  // indegree and mark as undecided. (As this function is
  // expected to initialise the scc_id array)
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const degree = graph.indegree(k);

    if (degree > 0) { // initialise
      scc_id[k]   = scc_id_undecided;
      indegree[k] = degree;
    }

    else { // decide right away and prepare to notify neighbours
      scc_id[k] = part.to_global(k);
      ++decided_count;
      for (auto const v : graph.csr_range(k)) {
        frontier.relaxed_push(part, v);
      }
    }
  }

  // With the initialisation done and the first collected decision we
  // can now simply delegate the exhaustive trimming to a sub routine.
  decided_count += trim_1_exhaustive(graph.fw_view(), scc_id, indegree, frontier);

  // == initial outdegree trim ==
  // We do similar to the initial indegree trim
  // but scc_id is already initialised and we might
  // already have decided vertices to skip.
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const degree = graph.outdegree(k);

      if (degree > 0) { // initialise
        outdegree[k] = degree;
      }

      else { // decide right away and prepare to notify neighbours
        scc_id[k] = part.to_global(k);
        ++decided_count;
        for (auto const v : graph.bw_csr_range(k)) {
          frontier.relaxed_push(part, v);
        }
      }
    }
  }

  // delegate the exhaustive trimming to a sub routine again.
  return decided_count + trim_1_exhaustive(graph.bw_view(), scc_id, outdegree, frontier);
}

} // namespace kaspan
