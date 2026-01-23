#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/scc/frontier.hpp>

namespace kaspan {

template<part_view_concept Part>
auto
trim_1_first(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  u64 max_degree_prod = 0;

  vertex_t decided = 0;
  index_t  pivot   = graph.part.n();

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    auto const outdegree = graph.outdegree(k);
    auto const indegree  = graph.indegree(k);

    if (indegree == 0 || outdegree == 0) {
      scc_id[k] = graph.part.to_global(k);
      ++decided;
    } else {
      scc_id[k]              = scc_id_undecided;
      auto const degree_prod = integral_cast<u64>(indegree) * outdegree;
      if (max_degree_prod < degree_prod) {
        max_degree_prod = degree_prod;
        pivot           = graph.part.to_global(k);
      }
    }
  }

  if (max_degree_prod != mpi_basic::allreduce_single(max_degree_prod, mpi_basic::max)) {
    // overwrite as not global max degree
    pivot = graph.part.n();
  }

  pivot = mpi_basic::allreduce_single(pivot, mpi_basic::min);

  return PACK(decided, pivot);
}

template<part_view_concept Part = single_part_view>
auto
trim_1_normal(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id)
{
  auto const has_degree = [&](graph_part_view<Part> g, auto k) {
    for (auto v : g.csr_range(k)) {
      if (!g.part.has_local(v) || scc_id[g.part.to_local(v)] != scc_id_undecided) return true;
    }
    return false;
  };

  vertex_t decided = 0;

  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
    if (!has_degree(graph.fw_view(), k) || !has_degree(graph.bw_view(), k)) {
      scc_id[k] = graph.part.to_global(k);
      ++decided;
    } else {
      scc_id[k] = scc_id_undecided;
    }
  }

  return decided;
}

template<bool              InterleavedSupport,
         part_view_concept Part>
auto
trim_1_first(
  bidi_graph_part_view<Part>        graph,
  vertex_t*                         scc_id,
  vertex_t*                         outdegree,
  vertex_t*                         indegree,
  frontier_view<vertex_t,
                InterleavedSupport> frontier)
{
  struct return_t
  {
    vertex_t decided_count = 0;
    degree_t max{};
  };

  auto       part          = graph.part;
  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;

  // initial trim forward
  for (vertex_t k = 0; k < local_n; ++k) {
    if (indegree[k] = graph.indegree(k); indegree[k] == 0) {
      scc_id[k] = part.to_global(k);
      ++decided_count;
      for (auto const v : graph.fw_csr_range(k)) {
        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    } else {
      scc_id[k] = scc_id_undecided;
    }
  }

  do {
    while (frontier.has_next()) {
      auto const k = part.to_local(frontier.next());
      if (scc_id[k] == scc_id_undecided) {
        if (--indegree[k]; indegree[k] == 0) {
          scc_id[k] = part.to_global(k);
          ++decided_count;
          for (auto const v : graph.fw_csr_range(k)) {
            if (part.has_local(v)) {
              frontier.local_push(v);
            } else {
              frontier.push(part.world_rank_of(v), v);
            }
          }
        }
      }
    }
  } while (frontier.comm(part));

  // initial trim backward
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      if (outdegree[k] = graph.outdegree(k); outdegree[k] == 0) {
        scc_id[k] = part.to_global(k);
        ++decided_count;
        for (auto const v : graph.bw_csr_range(k)) {
          if (part.has_local(v)) {
            frontier.local_push(v);
          } else {
            frontier.push(part.world_rank_of(v), v);
          }
        }
      }
    }
  }

  do {
    while (frontier.has_next()) {
      auto const k = part.to_local(frontier.next());
      if (scc_id[k] == scc_id_undecided) {
        if (--outdegree[k]; outdegree[k] == 0) {
          scc_id[k] = part.to_global(k);
          ++decided_count;
          for (auto const v : graph.bw_csr_range(k)) {
            if (part.has_local(v)) {
              frontier.local_push(v);
            } else {
              frontier.push(part.world_rank_of(v), v);
            }
          }
        }
      }
    }
  } while (frontier.comm(part));

  degree_t max{ .degree_product = std::numeric_limits<index_t>::min(), .u = scc_id_undecided };
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const prod = indegree[k] + outdegree[k];
      DEBUG_ASSERT_GT(prod, 0);

      if (prod > max.degree_product) {
        max.degree_product = prod;
        max.u              = part.to_global(k);
      }
    }
  }

  return return_t{ decided_count, max };
}

template<part_view_concept Part>
auto
trim_1(
  bidi_graph_part_view<Part> graph,
  vertex_t*                  scc_id,
  vertex_t const*            fw_degree,
  vertex_t const*            bw_degree) -> vertex_t
{
  auto       part          = graph.part;
  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      if (!fw_degree[k] || !bw_degree[k]) {
        scc_id[k] = part.to_global(k);
        ++decided_count;
      }
    }
  }
  return decided_count;
}

} // namespace kaspan
