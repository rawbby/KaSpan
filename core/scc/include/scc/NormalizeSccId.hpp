#pragma once

#include <buffer/Buffer.hpp>
#include <graph/Base.hpp>
#include <graph/Part.hpp>
#include <scc/Common.hpp>

template<WorldPartConcept Part>
void
normalize_scc_id(U64Buffer& scc_id, Part const& part)
{
  KASPAN_STATISTIC_SCOPE("normalize_scc_id");
  for (vertex_t k = 0; k < part.size(); ++k)
    if (scc_id[k] == scc_id_singular)
      scc_id[k] = part.select(k);
}
