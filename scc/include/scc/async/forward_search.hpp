#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <util/pp.hpp>

#include <scc/graph.hpp>
#include <scc/base.hpp>

#include <briefkasten/queue_builder.hpp>

#include <queue>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
void
forward_search(
  GraphPart<Part> const& graph,
  BriefQueue&            mq,
  vertex_t*              scc_id,
  BitAccessor            fw_reached,
  IF(KASPAN_NORMALIZE, vertex_t&, vertex_t) root)
{
  KASPAN_STATISTIC_SCOPE("forward_search");

  std::queue<vertex_t> local_q;

  auto on_message = [&](auto env) {
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

      if (fw_reached.get(k) || scc_id[k] != scc_id_undecided)
        continue;

      fw_reached.set(k);
      IF(KASPAN_NORMALIZE, root = std::min(root, u);)

      // push all forward neighbors
      auto const beg = graph.fw_head[k];
      auto const end = graph.fw_head[k + 1];
      for (auto it = beg; it < end; ++it) {
        auto const v = graph.fw_csr.get(it);
        if (graph.part.has_local(v)) {
          local_q.push(v);
        } else {
          mq.post_message_blocking(v, graph.part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }

  } while (mq.terminate(on_message));

#if KASPAN_NORMALIZE
  MPI_Allreduce(MPI_IN_PLACE, &root, 1, mpi_vertex_t, MPI_MIN, MPI_COMM_WORLD);
#endif
}

} // namespace async
