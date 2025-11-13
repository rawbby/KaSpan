#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <scc/base.hpp>
#include <util/pp.hpp>

template<WorldPartConcept Part>
void
forward_search(
  Part const&      part,
  index_t const*   fw_head,
  vertex_t const*  fw_csr,
  vertex_frontier& frontier,
  vertex_t const*  scc_id,
  BitAccessor      fw_reached,
  IF(KASPAN_NORMALIZE, vertex_t&, vertex_t) root)
{
  KASPAN_STATISTIC_SCOPE("forward_search");

  if (part.has_local(root)) {
    frontier.local_push(root);
    ASSERT(not fw_reached.get(part.to_local(root)));
    ASSERT(scc_id[part.to_local(root)] == scc_id_undecided, "root=%d", root);
  }

  for (;;) {
    size_t processed_count = 0;

    KASPAN_STATISTIC_PUSH("processing");
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = part.to_local(u);

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      ++processed_count;

      // now it is reached
      fw_reached.set(k);
      IF(KASPAN_NORMALIZE, root = std::min(root, u);)

      // add all neighbours to frontier
      auto const beg = fw_head[k];
      auto const end = fw_head[k + 1];
      for (auto i = beg; i < end; ++i) {
        auto const v = fw_csr[i];

        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
    KASPAN_STATISTIC_ADD("processed_count", processed_count);
    KASPAN_STATISTIC_POP();

    KASPAN_STATISTIC_PUSH("communication");
    KASPAN_STATISTIC_ADD("outbox", frontier.send_buffer.size());
    auto const messages_exchanged = frontier.comm(part);
    KASPAN_STATISTIC_ADD("inbox", frontier.recv_buffer.size());
    KASPAN_STATISTIC_POP();

    if (not messages_exchanged)
      break;
  }

#if KASPAN_NORMALIZE
  mpi_basic_allreduce_inplace(&root, 1, MPI_MIN);
#endif
}
