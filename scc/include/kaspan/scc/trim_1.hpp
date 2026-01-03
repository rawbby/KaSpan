#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

namespace kaspan {

template<world_part_concept part_t>
auto
trim_1_first(part_t const& part, index_t const* fw_head, index_t const* bw_head, vertex_t* scc_id)
{
  struct return_t
  {
    vertex_t decided_count = 0;
    degree   max{};
  };

  auto const local_n = part.local_n();

  vertex_t decided_count = 0;
  degree   max{ .degree_product = std::numeric_limits<index_t>::min(), .u = scc_id_undecided };

  for (vertex_t k = 0; k < local_n; ++k) {
    auto const out_degree = fw_head[k + 1] - fw_head[k];
    auto const in_degree  = bw_head[k + 1] - bw_head[k];

    if (out_degree == 0 or in_degree == 0) {
      scc_id[k] = part.to_global(k);
      ++decided_count;
      continue;
    }

    scc_id[k] = scc_id_undecided;

    auto const degree_product = out_degree * in_degree;
    if (degree_product >= max.degree_product) { max = { degree_product, part.to_global(k) }; }
  }

  return return_t{ decided_count, max };
}

template<world_part_concept part_t>
auto
trim_1(part_t const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id) -> vertex_t
{
  auto const has_degree = [=](vertex_t k, index_t const* head, vertex_t const* csr) -> bool {
    for (auto u : csr_range(head, csr, k)) {
      if (not part.has_local(u)) { return true; }
      if (scc_id[part.to_local(u)] == scc_id_undecided) { return true; }
    }
    return false;
  };

  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      if (not has_degree(k, fw_head, fw_csr) or not has_degree(k, bw_head, bw_csr)) {
        scc_id[k] = part.to_global(k);
        ++decided_count;
      }
    }
  }
  return decided_count;
}

} // namespace kaspan
