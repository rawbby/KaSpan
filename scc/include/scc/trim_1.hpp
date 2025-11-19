#pragma once

#include <memory/buffer.hpp>
#include <scc/base.hpp>
#include <scc/part.hpp>
#include <util/arithmetic.hpp>

template<WorldPartConcept Part>
auto
trim_1_first(Part const& part, index_t const* fw_head, index_t const* bw_head, vertex_t* scc_id)
{
  struct Return
  {
    vertex_t decided_count = 0;
    Degree   max{};
  };

  KASPAN_STATISTIC_SCOPE("trim_1_first");
  auto const local_n = part.local_n();

  vertex_t decided_count = 0;
  Degree   max{
      .degree_product = std::numeric_limits<index_t>::min(),
      .u              = scc_id_undecided
  };

  for (vertex_t k = 0; k < local_n; ++k) {
    DEBUG_ASSERT_EQ(scc_id[k], scc_id_undecided, "trim_1_first relies on completely undecided scc ids");

    auto const out_degree = fw_head[k + 1] - fw_head[k];
    auto const in_degree  = bw_head[k + 1] - bw_head[k];

    if (out_degree == 0 or in_degree == 0) {
      scc_id[k] = part.to_global(k);
      ++decided_count;
      continue;
    }

    auto const degree_product = out_degree * in_degree;
    if (degree_product >= max.degree_product) {
      max = { degree_product, part.to_global(k) };
    }
  }

  KASPAN_STATISTIC_ADD("decision_count", decided_count);
  return Return{ decided_count, max };
}
