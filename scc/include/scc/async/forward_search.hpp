#pragma once

#include <debug/assert.hpp>
#include <debug/statistic.hpp>
#include <memory/accessor/bits_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

#include <briefkasten/queue_builder.hpp>

#include <queue>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
auto
forward_search(
  Part const&      part,
  index_t const*   fw_head,
  vertex_t const*  fw_csr,
  BriefQueue&      mq,
  vertex_t const*  scc_id,
  BitsAccessor     fw_reached,
  vertex_t         root) -> vertex_t
{
  std::queue<vertex_t> local_q;

  auto on_message = [&](auto env) {
    for (auto&& v : env.message)
      local_q.push(v);
  };

  if (part.has_local(root)) {
    local_q.push(root);
    ASSERT(not fw_reached.get(part.to_local(root)));
    ASSERT(scc_id[part.to_local(root)] == scc_id_undecided, "root=%d", root);
  }

  do {
    while (!local_q.empty()) {
      auto const u = local_q.front();
      local_q.pop();

      DEBUG_ASSERT(part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = part.to_local(u);

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // now it is reached
      fw_reached.set(k);
      root = std::min(root, u);

      // push all forward neighbors
      for (vertex_t v : csr_range(fw_head, fw_csr, k)) {
        if (part.has_local(v)) {
          local_q.push(v);
        } else {
          mq.post_message_blocking(v, part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }

  } while (mq.terminate(on_message));

  return mpi_basic_allreduce_single(root, MPI_MIN);
}

} // namespace async
