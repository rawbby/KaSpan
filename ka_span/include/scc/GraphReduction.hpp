#pragma once

#include <buffer/Buffer.hpp>
#include <graph/DistributedGraph.hpp>
#include <scc/Common.hpp>
#include <util/Arithmetic.hpp>

#include <kamping/communicator.hpp>

static auto
gfq_origin(
  kamping::Communicator<>&  comm,
  DistributedGraph<> const& graph,
  U64Buffer const&          scc_id,

  I64Buffer& frontier_queue,
  I64Buffer& vert_map,

  I64Buffer& sub_fw_head,
  I64Buffer& sub_fw_csr,
  I64Buffer& sub_bw_head,
  I64Buffer& sub_bw_csr) -> VoidResult
{
  // create a subgraph that holds only the active nodes

  u64       sub_n = 0;
  U64Buffer local_frontier{};

  RESULT_TRY(auto sub_degree, U32Buffer::create(frontier_queue.size()));
  u64 sub_i = 0;

  // 1. build the frontier queue (dense list of active nodes)
  // + build a degree buffer that is later exchanged for creating the sub_head
  for (u64 global_u = 0; global_u < graph.node_count; ++global_u) {
    if (scc_id[global_u] == SCC_ID_UNDECIDED) {
      frontier_queue[sub_n] = global_u;
      vert_map[global_u]    = sub_n;

      if (graph.partition.contains(global_u)) {
        local_frontier[sub_i++] = sub_n; // store local frontier entries

        auto const local_u = graph.partition.rank(global_u);

        // calculate the active sub degree
        auto       csr_it  = graph.fw_head.get(local_u);
        auto const csr_end = graph.fw_head.get(local_u + 1);
        for (; csr_it < csr_end; ++csr_it) {
          auto const global_v = graph.fw_csr.get(csr_it);
          if (scc_id[global_v] == SCC_ID_UNDECIDED)
            ++sub_degree[sub_n];
        }
      }

      ++sub_n;
    }
  }

  // 2. synchronize the degree so that everyone has a global view
  MPI_Allreduce(MPI_IN_PLACE, sub_degree.data(), sub_n, MPI_UINT32_T, MPI_MAX, MPI_COMM_WORLD);

  // 3. create the sub_head from degree
  sub_fw_head[0] = 0;
  for (u64 i = 0; i < sub_n; ++i)
    sub_fw_head[i + 1] = sub_fw_head[i] + sub_degree[i];

  // 4. fill local csr entries
  for (auto const sub_u : local_frontier.range()) {
    auto const global_u = frontier_queue[sub_u];
    auto const local_u  = graph.partition.rank(global_u);

    auto       sub_csr_it = sub_fw_head[sub_u];
    auto       csr_it     = graph.fw_head.get(local_u);
    auto const csr_end    = graph.fw_head.get(local_u + 1);

    for (; csr_it < csr_end; ++csr_it) {
      auto const global_v = graph.fw_csr.get(csr_it);

      if (scc_id[global_v] == 0) { // active
        auto const sub_v       = vert_map[global_v];
        sub_fw_csr[sub_csr_it] = sub_v;
        ++sub_csr_it;
      }
    }
  }

  // 5. synchronize the csr entries so that everyone has a global view
  MPI_Allreduce(MPI_IN_PLACE, sub_fw_csr.data(), sub_fw_head[sub_n], MPI_UINT32_T, MPI_MAX, MPI_COMM_WORLD);

  // 6. todo build the sub bw graph from the sub fw graph without communication

  std::ranges::fill_n(sub_degree.range().begin(), sub_n, 0); // reuse sub_degree now for bw

  for (u64 u = 0; u < sub_n; ++u) {
    auto const begin = sub_fw_head[u];
    auto const end   = sub_fw_head[u + 1];

    for (auto it = begin; it < end; ++it) {
      auto const v = sub_fw_csr[it];
      sub_degree[v] += 1;
    }
  }

  // build head from degree and reinterpret sub_degree as iterator per adjacency list
  sub_bw_head[0] = 0;
  for (u64 i = 0; i < sub_n; ++i) {
    sub_bw_head[i + 1] = sub_bw_head[i] + sub_degree[i];
    sub_degree[i]      = sub_bw_head[i];
  }

  auto& adjacency_it = sub_degree; // make new interpretation explicit

  for (u64 u = 0; u < sub_n; ++u) {
    auto const begin = sub_fw_head[u];
    auto const end   = sub_fw_head[u + 1];

    for (auto it = begin; it < end; ++it) {
      auto const v = sub_fw_csr[it];

      sub_bw_csr[adjacency_it[v]] = u;
      ++adjacency_it[v];
    }
  }

  // todo needed?
  // front_comm[world_rank] = sub_n;
  // work_comm[world_rank]  = sub_fw_head[sub_n];

  return VoidResult::success();
}
