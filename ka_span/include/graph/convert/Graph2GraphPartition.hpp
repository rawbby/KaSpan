#pragma once

#include <util/Arithmetic.hpp>
#include <util/Result.hpp>
#include <util/Util.hpp>

#include <graph/DistributedGraph.hpp>
#include <graph/Graph.hpp>
#include <graph/Partition.hpp>

#include <utility>

template<WorldPartitionConcept Partition, bool Compress = false>
auto
load_local(Graph& g, Partition& partition) -> Result<DistributedGraph<Partition, Compress>>
{
  using DG          = DistributedGraph<Partition, Compress>;
  using buffer_type = DG::buffer_type;

  auto load_dir = [&](auto const& head, auto const& csr, buffer_type& out_head, buffer_type& out_csr) -> VoidResult {
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
    if constexpr (Compress) {
      std::cout << "    Step 3.x.0 - " << (local_n + 1) << ' ' << needed_bytes(local_m) << std::endl;
      RESULT_TRY(out_head, buffer_type::create(local_n + 1, needed_bytes(local_m)));
      std::cout << "    Step 3.x.1 - " << local_m << ' ' << needed_bytes(g.n ? g.n - 1 : 0) << std::endl;
      RESULT_TRY(out_csr, buffer_type::create(local_m, needed_bytes(g.n ? g.n - 1 : 0)));
      std::cout << "    Step 3.x.2" << std::endl;
    } else {
      std::cout << "    Step 3.x.0 - " << (local_n + 1) << std::endl;
      RESULT_TRY(out_head, buffer_type::create(local_n + 1));
      std::cout << "    Step 3.x.1 - " << local_m << std::endl;
      RESULT_TRY(out_csr, buffer_type::create(local_m));
      std::cout << "    Step 3.x.2" << std::endl;
    }
    u64 pos = 0;
    for (u64 k = 0; k < local_n; ++k) {
      auto const i = partition.select(k);
      auto const b = head[i];
      auto const e = head[i + 1];
      out_head.set(k, pos);
      for (auto it = b; it < e; ++it)
        out_csr.set(pos++, csr[it]);
    }
    out_head.set(local_n, pos);
    return VoidResult::success();
  };
  buffer_type fw_head, fw_csr, bw_head, bw_csr;
  std::cout << "    Step 3.0" << std::endl;
  RESULT_TRY(load_dir(g.fw_head, g.fw_csr, fw_head, fw_csr));
  std::cout << "    Step 3.1" << std::endl;
  RESULT_TRY(load_dir(g.bw_head, g.bw_csr, bw_head, bw_csr));
  std::cout << "    Step 3.2" << std::endl;
  DG result{ partition, g.n, g.m };
  result.fw_head = std::move(fw_head);
  result.fw_csr  = std::move(fw_csr);
  result.bw_head = std::move(bw_head);
  result.bw_csr  = std::move(bw_csr);
  return result;
}
