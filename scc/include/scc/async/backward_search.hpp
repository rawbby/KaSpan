#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <memory/accessor/bits_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

#include <briefkasten/buffered_queue.hpp>

#include <queue>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
auto
backward_search(
  Part const&      part,
  index_t const*   bw_head,
  vertex_t const*  bw_csr,
  BriefQueue&      mq,
  vertex_t*        scc_id,
  BitsAccessor     fw_reached,
  vertex_t         root,
  vertex_t         id) -> vertex_t
{
  vertex_t decided_count = 0;

  std::queue<vertex_t> local_q;

  auto on_message = [&](briefkasten::Envelope<vertex_t> auto env) {
    for (auto&& v : env.message)
      local_q.push(v);
  };

  if (part.has_local(root))
    local_q.push(root);

  do {
    while (!local_q.empty()) {
      auto const u = local_q.front();
      local_q.pop();

      DEBUG_ASSERT(part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = part.to_local(u);

      // skip if not in fw-reached or decided
      if (!fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = id;
      ++decided_count;

      // add all neighbours
      for (vertex_t v : csr_range(bw_head, bw_csr, k)) {
        if (part.has_local(v)) {
          local_q.push(v);
        } else {
          mq.post_message_blocking(v, part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }

  } while (mq.terminate(on_message));

  return decided_count;
}

} // namespace async
