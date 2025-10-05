#pragma once

#include <comm/SyncFrontierComm.hpp>
#include <graph/GraphPart.hpp>
#include <pp/PP.hpp>
#include <scc/Common.hpp>

#include <kamping/communicator.hpp>

template<WorldPartConcept Part>
void
sync_forward_search(
  kamping::Communicator<> const&              comm,
  GraphPart<Part> const&          graph,
  SyncAlltoallvBase<SyncFrontier<Part>>& frontier,
  U64Buffer const&                            scc_id,
  BitBuffer&                                  fw_reached,
  IF(KASPAN_NORMALIZE, u64&, u64) root)
{
  if (graph.part.contains(root)) {
    frontier.push_relaxed(root);
    ASSERT(not fw_reached.get(graph.part.rank(root)));
    ASSERT(scc_id[graph.part.rank(root)] == scc_id_undecided);
  }

  do { // NOLINT(*-avoid-do-while)

    while (frontier.has_next()) {
      auto const u = frontier.next();
      auto const k = graph.part.rank(u); // local index of u

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // now it is reached
      fw_reached.set(k);
      IF(KASPAN_NORMALIZE, root = std::min(root, u);)

      // add all neighbours to frontier
      auto const begin = graph.fw_head[k];
      auto const end   = graph.fw_head[k + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(graph.fw_csr.get(i));
    }
  } while (frontier.communicate(comm));
}

template<WorldPartConcept Part>
void
sync_backward_search(
  kamping::Communicator<> const&              comm,
  GraphPart<Part> const&          graph,
  SyncAlltoallvBase<SyncFrontier<Part>>& frontier,
  U64Buffer&                                  scc_id,
  BitBuffer const&                            fw_reached,
  u64                                         root,
  u64&                                        decided_count)
{
  if (graph.part.contains(root))
    frontier.push_relaxed(root);

  do { // NOLINT(*-avoid-do-while)

    while (frontier.has_next()) {
      auto const u = frontier.next();
      auto const k = graph.part.rank(u); // local index of u

      // skip if not in fw-reached or decided
      if (!fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = root;
      ++decided_count;

      // add all neighbours to frontier
      auto const begin = graph.bw_head[k];
      auto const end   = graph.bw_head[k + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(graph.bw_csr.get(i));
    }
  } while (frontier.communicate(comm));
}
