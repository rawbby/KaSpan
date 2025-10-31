#pragma once

#include <comm/SyncFrontierComm.hpp>
#include <debug/Assert.hpp>
#include <debug/Statistic.hpp>
#include <debug/Time.hpp>
#include <graph/GraphPart.hpp>
#include <scc/Common.hpp>

#include <kamping/communicator.hpp>

template<WorldPartConcept Part>
auto
backward_search(
  kamping::Communicator<> const&         comm,
  GraphPart<Part> const&                 graph,
  SyncAlltoallvBase<SyncFrontier<Part>>& frontier,
  U64Buffer&                             scc_id,
  BitBuffer const&                       fw_reached,
  u64                                    root) -> u64
{
  KASPAN_STATISTIC_SCOPE("backward_search");
  size_t decided_count = 0;

  if (graph.part.has_local(root))
    frontier.push_relaxed(root);

  for (;;) {
    size_t processed_count = 0;

    KASPAN_STATISTIC_PUSH("processing");
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(graph.part.has_local(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.to_local(u);

      // skip if not in fw-reached or decided
      if (!fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = root;
      ++processed_count;
      ++decided_count;

      // add all neighbours to frontier
      auto const begin = graph.bw_head[k];
      auto const end   = graph.bw_head[k + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(graph.bw_csr.get(i));
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

  KASPAN_STATISTIC_ADD("decided_count", decided_count);
  return decided_count;
}
