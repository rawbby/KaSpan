#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>
#include <kaspan/scc/vertex_frontier.hpp>

namespace kaspan {

/// trim_1_exhaustive iteratevely trims one direction, either forward or backward, exhaustive.
/// To do that scc_id must be valid and degree must be valid if:
/// scc_id[k] != undecided || valid(degree[k]).
template<world_part_concept part_t>
auto
trim_1_exhaustive(part_t const& part, index_t const* head, index_t const* csr, vertex_t* scc_id, vertex_t* degree, vertex_frontier& frontier) -> vertex_t
{
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
          for (auto const v : csr_range(head, csr, k)) {
            if (part.has_local(v)) frontier.local_push(v);
            else frontier.push(part.world_rank_of(v), v);
          }
        }
      }
    }
  } while (frontier.comm(part));

  return decided_count;
}

/// trim_1_exhaustive_first iteratively trims vertices with indegree/outdegree of zero.
/// It assumes to run on a fresh graph with uninitialised scc_id/indegree/outdegree and
/// will initilise these appropriately.
template<world_part_concept part_t>
auto
trim_1_exhaustive_first(part_t const&    part,
                        index_t const*   fw_head,
                        index_t const*   fw_csr,
                        index_t const*   bw_head,
                        index_t const*   bw_csr,
                        vertex_t*        scc_id,
                        vertex_t*        outdegree,
                        vertex_t*        indegree,
                        vertex_frontier& frontier) -> vertex_t
{
  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;

  // == initial indegree trim ==
  // We get the initial indegree right from the head array
  // and decide vertices with indegree of zero right away.
  // For every non resolved vertex we need to store the right
  // indegree and mark as undecided. (As this function is
  // expected to initialise the scc_id array)
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const degree = bw_head[k + 1] - bw_head[k];

    if (degree > 0) { // initialise
      scc_id[k]   = scc_id_undecided;
      indegree[k] = degree;
    }

    else { // decide right away and prepare to notify neighbours
      scc_id[k] = part.to_global(k);
      ++decided_count;
      for (auto const v : csr_range(fw_head, fw_csr, k)) {
        if (part.has_local(v)) frontier.local_push(v);
        else frontier.push(part.world_rank_of(v), v);
      }
    }
  }

  // With the initialisation done and the first collected decision we
  // can now simply delegate the exhaustive trimming to a sub routine.
  decided_count += trim_1_exhaustive(part, fw_head, fw_csr, scc_id, indegree, frontier);

  // == initial outdegree trim ==
  // We do similar to the initial indegree trim
  // but scc_id is already initialised and we might
  // already have decided vertices to skip.
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const degree = fw_head[k + 1] - fw_head[k];

      if (degree > 0) { // initialise
        outdegree[k] = degree;
      }

      else { // decide right away and prepare to notify neighbours
        scc_id[k] = part.to_global(k);
        ++decided_count;
        for (auto const v : csr_range(bw_head, bw_csr, k)) {
          if (part.has_local(v)) frontier.local_push(v);
          else frontier.push(part.world_rank_of(v), v);
        }
      }
    }
  }

  // delegate the exhaustive trimming to a sub routine again.
  return decided_count + trim_1_exhaustive(part, fw_head, fw_csr, scc_id, indegree, frontier);
}

} // namespace kaspan
