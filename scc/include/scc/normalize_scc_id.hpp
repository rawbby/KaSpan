#pragma once

#include <debug/statistic.hpp>
#include <scc/base.hpp>
#include <scc/part.hpp>
#include <memory/buffer.hpp>
#include <scc/base.hpp>

template<WorldPartConcept Part>
void
normalize_scc_id(Part const& part, vertex_t* scc_id)
{
  KASPAN_STATISTIC_SCOPE("normalize_scc_id");
  for (vertex_t k = 0; k < part.local_n(); ++k)
    if (scc_id[k] == scc_id_singular)
      scc_id[k] = part.to_global(k);
}
