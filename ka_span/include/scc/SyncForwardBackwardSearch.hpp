#pragma once

#include <comm/SyncFrontierComm.hpp>
#include <graph/DistributedGraph.hpp>
#include <scc/Common.hpp>
#include <util/Time.hpp>

#include <kamping/communicator.hpp>

#include <cstdint>
#include <numeric>
#include <set>

/**
 * forward reachability via sync bfs.
 * (relaxed: allows local DFS within a level)
 */
template<WorldPartitionConcept Partition>
void
sync_forward_search(
  kamping::Communicator<> const&     comm,
  DistributedGraph<Partition> const& graph,
  SyncFrontierComm<Partition>&       frontier,
  U64Buffer const&                   scc_id,
  BitBuffer&                         fw_reached,
  u64                                root)
{
  if (graph.partition.contains(root)) {
    frontier.push_relaxed(comm, graph.partition, root);
    fw_reached.set(graph.partition.rank(root));
  }

  do { // NOLINT(*-avoid-do-while)

    while (frontier.has_next()) {
      auto const u = frontier.next();
      auto const k = graph.partition.rank(u); // local index of u

      // skip if reached or decided
      if (fw_reached.get(k) or scc_id[k] != SCC_ID_UNDECIDED)
        continue;

      // now it is reached
      fw_reached.set(k);

      // add all neighbours to frontier
      auto const begin = graph.fw_head[u];
      auto const end   = graph.fw_head[u + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(comm, graph.partition, graph.fw_csr.get(i));
    }
  } while (frontier.communicate(comm, graph.partition));
}

/**
 * backward reachability restricted to forward-reached via sync bfs.
 * (relaxed: allows local DFS within a level)
 */
template<WorldPartitionConcept Partition>
void
sync_backward_search(
  kamping::Communicator<> const&     comm,
  DistributedGraph<Partition> const& graph,
  SyncFrontierComm<Partition>&       frontier,
  U64Buffer&                         scc_id,
  BitBuffer const&                   fw_reached,
  u64                                root)
{
  if (graph.partition.contains(root) && fw_reached.get(graph.partition.rank(root))) {
    frontier.push_relaxed(comm, graph.partition, root);
    scc_id[graph.partition.rank(root)] = root;
  }

  do { // NOLINT(*-avoid-do-while)

    while (frontier.has_next()) {
      auto const u = frontier.next();
      auto const k = graph.partition.rank(u); // local index of u

      // skip if not in fw-reached or decided
      if (!fw_reached.get(k) or scc_id[k] != SCC_ID_UNDECIDED)
        continue;

      // (inside fw-reached and bw-reached => contributes to scc)
      scc_id[k] = root;

      // add all neighbours to frontier
      auto const begin = graph.bw_head[u];
      auto const end   = graph.bw_head[u + 1];
      for (auto i = begin; i < end; ++i)
        frontier.push_relaxed(comm, graph.partition, graph.bw_csr.get(i));
    }
  } while (frontier.communicate(comm, graph.partition));
}

inline auto
mice_fw_bw(
  kamping::Communicator<> const& comm,
  I64Buffer const&               wcc_color,
  I64Buffer&                     scc_id,
  I64Buffer const&               sub_fw_beg,
  I64Buffer const&               sub_bw_beg,
  I64Buffer const&               sub_fw_csr,
  I64Buffer const&               sub_bw_csr,
  I64Buffer&                     fw_sa,
  i64                            sub_v_count,
  I64Buffer const&               wcc_fq,
  i64                            wcc_fq_size) -> VoidResult
{
  i64 const step    = wcc_fq_size / comm.size();
  i64 const wcc_beg = comm.rank() * step;
  i64 const wcc_end = comm.rank() == comm.size() - 1 ? wcc_fq_size : wcc_beg + step;

  RESULT_TRY(auto q, I64Buffer::create(sub_v_count));
  i64 head = 0;
  i64 tail = 0;

  for (i64 v = 0; v < sub_v_count; ++v) {
    if (scc_id[v] == SCC_ID_SINGLE) {
      i64 const cur_wcc = wcc_color[v];
      bool      in_wcc  = false;
      for (i64 i = wcc_beg; i < wcc_end; ++i) {
        if (wcc_fq[i] == cur_wcc) {
          in_wcc = true;
          break;
        }
      }
      if (in_wcc) {
        fw_sa[v]  = v;
        q[tail++] = v;
        if (tail == sub_v_count)
          tail = 0;

        while (head != tail) {
          i64 temp_v = q[head++];

          if (head == sub_v_count)
            head = 0;
          i64 my_beg = sub_fw_beg[temp_v];
          i64 my_end = sub_fw_beg[temp_v + 1];

          for (; my_beg < my_end; ++my_beg) {
            i64 w = sub_fw_csr[my_beg];

            if (fw_sa[w] != v) {
              q[tail++] = w;
              if (tail == sub_v_count)
                tail = 0;
              fw_sa[w] = v;
            }
          }
        }

        scc_id[v] = v;
        q[tail++] = v;
        if (tail == sub_v_count)
          tail = 0;

        while (head != tail) {
          i64 temp_v = q[head++];

          if (head == sub_v_count)
            head = 0;
          i64 my_beg = sub_bw_beg[temp_v];
          i64 my_end = sub_bw_beg[temp_v + 1];

          for (; my_beg < my_end; ++my_beg) {
            i64 w = sub_bw_csr[my_beg];

            if (scc_id[w] == SCC_ID_SINGLE && fw_sa[w] == v) {
              q[tail++] = w;
              if (tail == sub_v_count)
                tail = 0;
              scc_id[w] = v;
            }
          }
        }
      }
    }
  }

  return VoidResult::success();
}
