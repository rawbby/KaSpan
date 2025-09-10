#pragma once

#include <comm/SyncFrontierComm.hpp>
#include <graph/DistributedGraph.hpp>
#include <scc/Common.hpp>

#include <kamping/communicator.hpp>

template<WorldPartitionConcept Partition>
void
sync_forward_search(
  kamping::Communicator<> const&              comm,
  DistributedGraph<Partition> const&          graph,
  SyncAlltoallvBase<SyncFrontier<Partition>>& frontier,
  U64Buffer const&                            scc_id,
  BitBuffer&                                  fw_reached,
  u64                                         root)
{
  if (graph.partition.contains(root)) {
    frontier.push_relaxed(root);
    fw_reached.set(graph.partition.rank(root));
  }

  do { // NOLINT(*-avoid-do-while)

    while (frontier.has_next()) {
      auto const u = frontier.next();
      auto const k = graph.partition.rank(u); // local index of u

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // now it is reached
      fw_reached.set(k);

      // add all neighbours to frontier
      auto const begin = graph.fw_head[u];
      auto const end   = graph.fw_head[u + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(graph.fw_csr.get(i));
    }
  } while (frontier.communicate(comm));
}

template<WorldPartitionConcept Partition>
void
sync_backward_search(
  kamping::Communicator<> const&              comm,
  DistributedGraph<Partition> const&          graph,
  SyncAlltoallvBase<SyncFrontier<Partition>>& frontier,
  U64Buffer&                                  scc_id,
  BitBuffer const&                            fw_reached,
  u64                                         root,
  u64&                                        decided_count)
{
  if (graph.partition.contains(root) && fw_reached.get(graph.partition.rank(root))) {
    frontier.push_relaxed(root);
    scc_id[graph.partition.rank(root)] = root;
    ++decided_count;
  }

  do { // NOLINT(*-avoid-do-while)

    while (frontier.has_next()) {
      auto const u = frontier.next();
      auto const k = graph.partition.rank(u); // local index of u

      // skip if not in fw-reached or decided
      if (!fw_reached.get(k) or scc_id[k] != scc_id_undecided)
        continue;

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = root;
      ++decided_count;

      // add all neighbours to frontier
      auto const begin = graph.bw_head[u];
      auto const end   = graph.bw_head[u + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(graph.bw_csr.get(i));
    }
  } while (frontier.communicate(comm));
}
