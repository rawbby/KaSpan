#pragma once

#include <util/Arithmetic.hpp>
#include <util/Result.hpp>
#include <util/Util.hpp>

#include <graph/DistributedGraph.hpp>
#include <graph/Graph.hpp>
#include <graph/Partition.hpp>

#include <utility>

template<WorldPartitionConcept Partition>
auto
load_local(Graph& g, Partition partition) -> Result<DistributedGraph<Partition>>
{
  using DG = DistributedGraph<Partition>;

  auto load_dir = [&](auto const& head, auto const& csr, VertexBuffer& out_head, IndexBuffer& out_csr) -> VoidResult {
    u64 const local_n = partition.size();
    u64       local_m = 0;
    if constexpr (Partition::continuous) {
      if (local_n > 0)
        local_m = head[partition.end] - head[partition.begin];
    } else {
      for (u64 k = 0; k < local_n; ++k) {
        auto const i = partition.select(k);
        local_m += head[i + 1] - head[i];
      }
    }

    RESULT_TRY(out_head, create_head_buffer(partition, local_m));
    RESULT_TRY(out_csr, create_csr_buffer(partition, local_m));

    u64 pos = 0;
    for (u64 k = 0; k < local_n; ++k) {
      auto const u = partition.select(k);

      out_head.set(k, pos);
      for (auto v = head[u]; v < head[u + 1]; ++v)
        out_csr.set(pos++, csr[v]);
    }
    out_head.set(local_n, pos);
    return VoidResult::success();
  };

  DG result{};
  RESULT_TRY(load_dir(g.fw_head, g.fw_csr, result.fw_head, result.fw_csr));
  RESULT_TRY(load_dir(g.bw_head, g.bw_csr, result.bw_head, result.bw_csr));
  result.partition = std::move(partition);
  result.n         = g.n;
  result.m         = g.m;
  return result;
}
