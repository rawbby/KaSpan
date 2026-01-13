#pragma once

#include <kaspan/scc/vertex_frontier.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

namespace kaspan {

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
                        vertex_frontier& frontier)
{
  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;

  // initial trim forward
  for (vertex_t k = 0; k < local_n; ++k) {
    if (indegree[k] = bw_head[k + 1] - bw_head[k]; indegree[k] == 0) {
      scc_id[k] = part.to_global(k);
      ++decided_count;
      for (auto const v : csr_range(fw_head, fw_csr, k)) {
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
          for (auto const v : csr_range(fw_head, fw_csr, k)) {
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
      if (outdegree[k] = fw_head[k + 1] - fw_head[k]; outdegree[k] == 0) {
        scc_id[k] = part.to_global(k);
        ++decided_count;
        for (auto const v : csr_range(bw_head, bw_csr, k)) {
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
          for (auto const v : csr_range(bw_head, bw_csr, k)) {
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

  degree max_degree{ .degree_product = std::numeric_limits<index_t>::min(), .u = scc_id_undecided };
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const prod = indegree[k] + outdegree[k];
      DEBUG_ASSERT_GT(prod, 0);

      if (prod > max_degree.degree_product) {
        max_degree.degree_product = prod;
        max_degree.u              = part.to_global(k);
      }
    }
  }

  return PACK(decided_count, max_degree);
}

template<world_part_concept part_t>
auto
trim_1_exhaustive(part_t const&   part,
                  index_t const*  fw_head,
                  vertex_t const* fw_csr,
                  index_t const*  bw_head,
                  vertex_t const* bw_csr,
                  vertex_t*       scc_id,
                  vertex_t*       fw_degree,
                  vertex_t*       bw_degree) -> vertex_t
{
  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      if (not fw_degree[k] or not bw_degree[k]) {
        scc_id[k] = part.to_global(k);
        ++decided_count;
      }
    }
  }
  return decided_count;
}

} // namespace kaspan
