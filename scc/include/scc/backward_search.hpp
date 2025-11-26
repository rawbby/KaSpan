#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <memory/bit_accessor.hpp>
#include <scc/base.hpp>
#include <scc/vertex_frontier.hpp>

template<WorldPartConcept Part>
auto
backward_search(
  Part const&      part,
  index_t const*   bw_head,
  vertex_t const*  bw_csr,
  vertex_frontier& frontier,
  vertex_t*        scc_id,
  BitAccessor      fw_reached,
  vertex_t         root,
  vertex_t         id) -> vertex_t
{
  // KASPAN_STATISTIC_SCOPE("backward_search");
  vertex_t decided_count = 0;

  if (part.has_local(root))
    frontier.local_push(root);

  for (;;) {
    size_t processed_count = 0;

    // KASPAN_STATISTIC_PUSH("processing");
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = part.to_local(u);

      // skip if not in fw-reached or decided
      if (!fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = id;
      ++processed_count;
      ++decided_count;

      // add all neighbours to frontier
      auto const begin = bw_head[k];
      auto const end   = bw_head[k + 1];
      for (auto i = begin; i < end; ++i) {
        auto const v = bw_csr[i];

        if (part.has_local(v)) {
          frontier.local_push(v);
        } else {
          frontier.push(part.world_rank_of(v), v);
        }
      }
    }
    // KASPAN_STATISTIC_ADD("processed_count", processed_count);
    // KASPAN_STATISTIC_POP();

    // KASPAN_STATISTIC_PUSH("communication");
    // KASPAN_STATISTIC_ADD("outbox", frontier.send_buffer.size());
    auto const total_messages = frontier.comm(part);
    // KASPAN_STATISTIC_ADD("inbox", frontier.recv_buffer.size());
    // KASPAN_STATISTIC_POP();

    if (not total_messages)
      break;
  }

  // KASPAN_STATISTIC_ADD("decided_count", decided_count);
  return decided_count;
}
