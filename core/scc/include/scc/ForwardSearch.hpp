#pragma once

#include <comm/SyncFrontierComm.hpp>
#include <debug/Assert.hpp>
#include <debug/Statistic.hpp>
#include <debug/Time.hpp>
#include <graph/GraphPart.hpp>
#include <pp/PP.hpp>
#include <scc/Common.hpp>

#include <kamping/communicator.hpp>

#include <queue>

template<WorldPartConcept Part>
void
forward_search(
  kamping::Communicator<> const&         comm,
  GraphPart<Part> const&                 graph,
  SyncAlltoallvBase<SyncFrontier<Part>>& frontier,
  U64Buffer const&                       scc_id,
  BitBuffer&                             fw_reached,
  IF(KASPAN_NORMALIZE, u64&, u64) root)
{
  KASPAN_STATISTIC_SCOPE("forward_search");

  if (graph.part.contains(root)) {
    frontier.push_relaxed(root);
    ASSERT(not fw_reached.get(graph.part.rank(root)));
    ASSERT(scc_id[graph.part.rank(root)] == scc_id_undecided, " for root=%lu", root);
  }

  for (;;) {
    size_t processed_count = 0;

    KASPAN_STATISTIC_PUSH("processing");
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(graph.part.contains(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.rank(u);

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      ++processed_count;

      // now it is reached
      fw_reached.set(k);
      IF(KASPAN_NORMALIZE, root = std::min(root, u);)

      // add all neighbours to frontier
      auto const begin = graph.fw_head[k];
      auto const end   = graph.fw_head[k + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(graph.fw_csr.get(i));
    }
    KASPAN_STATISTIC_ADD("processed_count", processed_count);
    KASPAN_STATISTIC_POP();

    KASPAN_STATISTIC_PUSH("communication");
    KASPAN_STATISTIC_ADD("outbox", frontier.send_buf().size());
    auto const messages_exchanged = frontier.communicate(comm);
    KASPAN_STATISTIC_ADD("inbox", frontier.recv_buf().size());
    KASPAN_STATISTIC_POP();


    if (not messages_exchanged)
      break;
  }

  IF(KASPAN_NORMALIZE, root = comm.allreduce_single(kamping::send_buf(root), kamping::op(MPI_MIN)));
}
