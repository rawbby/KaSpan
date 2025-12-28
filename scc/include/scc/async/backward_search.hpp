#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <util/pp.hpp>

#include <briefkasten/buffered_queue.hpp>

#include <queue>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
auto
backward_search(
  GraphPart<Part> const& graph,
  BriefQueue&            mq,
  vertex_t*              scc_id,
  BitAccessor            fw_reached,
  vertex_t               root) -> vertex_t
{
  KASPAN_STATISTIC_SCOPE("backward_search");
  vertex_t decided_count = 0;

  std::queue<vertex_t> local_q;

  auto on_message = [&](briefkasten::Envelope<vertex_t> auto env) {
    for (auto&& v : env.message)
      local_q.push(v);
  };

  if (graph.part.has_local(root))
    local_q.push(root);

  do {
    while (!local_q.empty()) {
      auto const u = local_q.front();
      local_q.pop();

      DEBUG_ASSERT(graph.part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.to_local(u);

      if (!fw_reached.get(k) || scc_id[k] != scc_id_undecided)
        continue;

      scc_id[k] = root;
      ++decided_count;

      auto const begin = graph.bw_head[k];
      auto const end   = graph.bw_head[k + 1];
      for (auto i = begin; i < end; ++i) {
        auto const v = graph.bw_csr.get(i);
        if (graph.part.has_local(v)) {
          local_q.push(v);
        } else {
          mq.post_message_blocking(v, graph.part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }

  } while (mq.terminate(on_message));

  KASPAN_STATISTIC_ADD("decided_count", decided_count);
  return decided_count;
}

} // namespace async
