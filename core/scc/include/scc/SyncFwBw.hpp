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

template<WorldPartConcept Part>
void
sync_forward_search(
  kamping::Communicator<> const&         comm,
  GraphPart<Part> const&                 graph,
  SyncAlltoallvBase<SyncFrontier<Part>>& frontier,
  U64Buffer const&                       scc_id,
  BitBuffer&                             fw_reached,
  IF(KASPAN_NORMALIZE, u64&, u64) root)
{
  if (graph.part.contains(root)) {
    frontier.push_relaxed(root);
    ASSERT(not fw_reached.get(graph.part.rank(root)));
    ASSERT(scc_id[graph.part.rank(root)] == scc_id_undecided, " for root=%lu", root);
  }

  for (;;) {

    IF(KASPAN_STATISTIC, size_t processed_count = 0;)
    kaspan_statistic_push("processing");
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(graph.part.contains(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.rank(u);

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

      IF(KASPAN_STATISTIC, ++processed_count;)
    }
    kaspan_statistic_pop();

    kaspan_statistic_add("processed_count", processed_count);
    kaspan_statistic_add("outbox", frontier.send_buf().size());

    kaspan_statistic_push("communication");
    auto const messages_exchanged = frontier.communicate(comm);
    kaspan_statistic_pop();

    kaspan_statistic_add("inbox", frontier.recv_buf().size());

    if (not messages_exchanged)
      break;
  }
}

template<WorldPartConcept Part>
void
sync_backward_search(
  kamping::Communicator<> const&         comm,
  GraphPart<Part> const&                 graph,
  SyncAlltoallvBase<SyncFrontier<Part>>& frontier,
  U64Buffer&                             scc_id,
  BitBuffer const&                       fw_reached,
  u64                                    root,
  u64&                                   decided_count)
{
  if (graph.part.contains(root))
    frontier.push_relaxed(root);

  for (;;) {

    IF(KASPAN_STATISTIC, size_t processed_count = 0;)
    kaspan_statistic_push("processing");
    while (frontier.has_next()) {
      auto const u = frontier.next();
      DEBUG_ASSERT(graph.part.contains(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.rank(u);

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

      IF(KASPAN_STATISTIC, ++processed_count;)
    }
    kaspan_statistic_pop();

    kaspan_statistic_add("processed_count", processed_count);
    kaspan_statistic_add("outbox", frontier.send_buf().size());

    kaspan_statistic_push("communication");
    auto const messages_exchanged = frontier.communicate(comm);
    kaspan_statistic_pop();

    kaspan_statistic_add("inbox", frontier.recv_buf().size());

    if (not messages_exchanged)
      break;
  }
}

template<WorldPartConcept Part, typename BriefQueue>
void
async_forward_search(
  GraphPart<Part> const& graph,
  BriefQueue&            mq,
  U64Buffer const&       scc_id,
  BitBuffer&             fw_reached,
  IF(KASPAN_NORMALIZE, u64&, u64) root)
{
  std::queue<vertex_t> local_q;

  auto on_message = [&](auto env) {
    for (auto&& v : env.message)
      local_q.push(v);
  };

  if (graph.part.contains(root))
    local_q.push(root);

  for (;;) {

    IF(KASPAN_STATISTIC, size_t processed_count = 0;)
    kaspan_statistic_push("processing");
    while (!local_q.empty()) {
      auto const u = local_q.front();
      local_q.pop();

      DEBUG_ASSERT(graph.part.contains(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.rank(u);

      if (fw_reached.get(k) || scc_id[k] != scc_id_undecided)
        continue;

      IF(KASPAN_STATISTIC, ++processed_count;)

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
    kaspan_statistic_pop();

    IF(KASPAN_STATISTIC, if (processed_count))
    {
      kaspan_statistic_add("processed_count", processed_count);
    }

    kaspan_statistic_push("communication");
    auto const finished = mq.terminate(on_message);
    kaspan_statistic_pop();

    if (finished)
      break;
  }
}

template<WorldPartConcept Part, typename BriefQueue>
void
async_backward_search(
  GraphPart<Part> const& graph,
  BriefQueue&            mq,
  U64Buffer&             scc_id,
  BitBuffer const&       fw_reached,
  u64                    root,
  u64&                   decided_count)
{
  std::queue<vertex_t> local_q;

  auto on_message = [&](briefkasten::Envelope<vertex_t> auto env) {
    for (auto&& v : env.message)
      local_q.push(v);
  };

  if (graph.part.contains(root))
    local_q.push(root);

  for (;;) {

    IF(KASPAN_STATISTIC, size_t processed_count = 0;)
    kaspan_statistic_push("processing");
    while (!local_q.empty()) {
      auto const u = local_q.front();
      local_q.pop();

      DEBUG_ASSERT(graph.part.contains(u), "It should be impossible to receive a vertex that is not contained in the ranks partition");
      auto const k = graph.part.rank(u);

      if (!fw_reached.get(k) || scc_id[k] != scc_id_undecided)
        continue;

      IF(KASPAN_STATISTIC, ++processed_count;)

      scc_id[k] = root;
      ++decided_count;

      auto const begin = graph.bw_head[k];
      auto const end   = graph.bw_head[k + 1];
      for (auto i = begin; i < end; ++i) {
        auto const v = graph.bw_csr.get(i);
        if (graph.part.contains(v)) {
          local_q.push(v);
        } else {
          mq.post_message_blocking(v, graph.part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }
    kaspan_statistic_pop();

    IF(KASPAN_STATISTIC, if (processed_count))
    {
      kaspan_statistic_add("processed_count", processed_count);
    }

    kaspan_statistic_push("communication");
    auto const finished = mq.terminate(on_message);
    kaspan_statistic_pop();

    if (finished)
      break;
  }
}
