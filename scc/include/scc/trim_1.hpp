#pragma once

#include <scc/base.hpp>
#include <scc/part.hpp>

template<WorldPartConcept Part>
auto
trim_1_first(Part const& part, index_t const* fw_head, index_t const* bw_head, vertex_t* scc_id)
{
  struct Return
  {
    vertex_t decided_count = 0;
    Degree   max{};
  };

  auto const local_n = part.local_n();

  vertex_t decided_count = 0;
  Degree   max{
      .degree_product = std::numeric_limits<index_t>::min(),
      .u              = scc_id_undecided
  };

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
    if (degree_product >= max.degree_product) {
      max = { degree_product, part.to_global(k) };
    }
  }

  return Return{ decided_count, max };
}

template<WorldPartConcept Part>
auto
trim_1(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id) -> vertex_t
{
  auto const has_degree = [=](vertex_t k, index_t const* head, vertex_t const* csr) -> bool {
    for (auto u : csr_range(head, csr, k)) {
      if (not part.has_local(u)) {
        return true;
      }
      if (scc_id[k] == scc_id_undecided) {
        return true;
      }
    }
    return false;
  };

  auto const local_n       = part.local_n();
  vertex_t   decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (not has_degree(k, fw_head, fw_csr)) {
      scc_id[k] = part.to_global(k);
      ++decided_count;
    }
    if (not has_degree(k, bw_head, bw_csr)) {
      scc_id[k] = part.to_global(k);
      ++decided_count;
    }
  }
  return decided_count;
}
