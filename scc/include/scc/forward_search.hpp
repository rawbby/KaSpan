#pragma once

#include <scc/base.hpp>

template<WorldPartConcept Part>
void
forward_search(
  Part const&      part,
  index_t const*   fw_head,
  vertex_t const*  fw_csr,
  vertex_frontier& frontier,
  vertex_t const*  scc_id,
  BitsAccessor     fw_reached,
  vertex_t         pivot)
{
  DEBUG_ASSERT_NOT(frontier.has_next());
  if (part.has_local(pivot)) {
    DEBUG_ASSERT(scc_id[part.to_local(pivot)] == scc_id_undecided, "pivot={}", pivot);
    frontier.local_push(pivot);
  }

  do {
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(part.has_local(u));
      auto const k = part.to_local(u);

      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      fw_reached.set(k);

      for (vertex_t v : csr_range(fw_head, fw_csr, k)) {
        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
  } while (frontier.comm(part));
}
