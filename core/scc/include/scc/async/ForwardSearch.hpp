#pragma once

#include <comm/SyncFrontierComm.hpp>
#include <debug/Assert.hpp>
#include <debug/Statistic.hpp>
#include <debug/Time.hpp>
#include <graph/GraphPart.hpp>
#include <pp/PP.hpp>
#include <scc/Common.hpp>

#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/queue_builder.hpp>

#include <kamping/communicator.hpp>

#include <queue>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
void
forward_search(
  kamping::Communicator<> & comm,
  GraphPart<Part> const& graph,
  BriefQueue&            mq,
  U64Buffer const&       scc_id,
  BitBuffer&             fw_reached,
  IF(KASPAN_NORMALIZE, u64&, u64) root)
{
  KASPAN_STATISTIC_SCOPE("forward_search");

  std::queue<vertex_t> local_q;

  auto on_message = [&](auto env) {
    for (auto&& v : env.message)
      local_q.push(v);
  };

  if (graph.part.contains(root))
    local_q.push(root);

  for (;;) {
    size_t processed_count = 0;

    KASPAN_STATISTIC_PUSH("processing");
    while (!local_q.empty()) {
      auto const u = local_q.front();
      local_q.pop();

      DEBUG_ASSERT(graph.part.contains(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.rank(u);

      if (fw_reached.get(k) || scc_id[k] != scc_id_undecided)
        continue;

      ++processed_count;

      fw_reached.set(k);
      IF(KASPAN_NORMALIZE, root = std::min(root, u);)

      // push all forward neighbors
      auto const beg = graph.fw_head[k];
      auto const end = graph.fw_head[k + 1];
      for (auto it = beg; it < end; ++it) {
        auto const v = graph.fw_csr.get(it);
        if (graph.part.contains(v)) {
          local_q.push(v);
        } else {
          mq.post_message_blocking(v, graph.part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }
    KASPAN_STATISTIC_POP();

    IF_KASPAN_STATISTIC(if (processed_count))
    {
      KASPAN_STATISTIC_ADD("processed_count", processed_count);
    }

    KASPAN_STATISTIC_PUSH("communication");
    auto const finished = mq.terminate(on_message);
    KASPAN_STATISTIC_POP();

    if (finished)
      break;
  }

  IF(KASPAN_NORMALIZE, root = comm.allreduce_single(kamping::send_buf(root), kamping::op(MPI_MIN)));
}

} // namespace async
