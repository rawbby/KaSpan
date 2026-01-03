#pragma once

#include <kaspan/debug/statistic.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

namespace kaspan {

template<world_part_concept part_t>
void
normalize_scc_id(part_t const& part, vertex_t* scc_id)
{
  KASPAN_STATISTIC_SCOPE("normalize_scc_id");
  // for (vertex_t k = 0; k < part.local_n(); ++k)
  //   if (scc_id[k] == scc_id_singular)
  //     scc_id[k] = part.to_global(k);
}

} // namespace kaspan
