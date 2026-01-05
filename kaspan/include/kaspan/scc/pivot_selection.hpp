#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

namespace kaspan {

inline auto
pivot_selection(degree max_degree) -> vertex_t
{
  return mpi_basic::allreduce_single(max_degree, mpi_degree_t, mpi_degree_max_op).u;
}

template<world_part_concept part_t>
auto
pivot_selection(part_t const& part, index_t const* fw_head, index_t const* bw_head, vertex_t const* scc_id) -> vertex_t
{
  degree max_degree{ .degree_product = std::numeric_limits<index_t>::min(), .u = std::numeric_limits<vertex_t>::min() };

  for (vertex_t k = 0; k < part.local_n(); ++k) {
    if (scc_id[k] == scc_id_undecided) {
      auto const out_degree     = fw_head[k + 1] - fw_head[k];
      auto const in_degree      = bw_head[k + 1] - bw_head[k];
      auto const degree_product = out_degree * in_degree;

      if (degree_product > max_degree.degree_product) [[unlikely]] {
        max_degree.degree_product = degree_product;
        max_degree.u              = part.to_global(k);
      }
    }
  }
  return pivot_selection(max_degree);
}

} // namespace kaspan
