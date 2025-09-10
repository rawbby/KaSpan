#pragma once

#include <buffer/Buffer.hpp>
#include <comm/SyncEdgeComm.hpp>
#include <graph/Manifest.hpp>
#include <graph/Partition.hpp>
#include <util/Arithmetic.hpp>
#include <util/Result.hpp>
#include <util/Util.hpp>

#include <cstring>
#include <filesystem>
#include <kagen.h>
#include <kamping/communicator.hpp>

struct Graph
{
  u64 n;
  u64 m;

  U64Buffer fw_head;
  U64Buffer bw_head;
  U64Buffer fw_csr;
  U64Buffer bw_csr;

  Graph(u64 n, u64 m)
    : n(n)
    , m(m)
  {
  }

  auto operator==(Graph const&) const -> bool = default;
  auto operator!=(Graph const&) const -> bool = default;

  static auto create(u64 n, u64 m) -> Result<Graph>
  {
    Graph result{ n, m };
    RESULT_TRY(result.fw_head, U64Buffer::create(n + 1));
    RESULT_TRY(result.fw_csr, U64Buffer::create(m));
    RESULT_TRY(result.bw_head, U64Buffer::create(n + 1));
    RESULT_TRY(result.bw_csr, U64Buffer::create(m));
    return result;
  }

  [[nodiscard]] auto fw_degree(u64 k) const -> u64
  {
    return fw_head[k + 1] - fw_head[k];
  }

  [[nodiscard]] auto bw_degree(u64 k) const -> u64
  {
    return bw_head[k + 1] - bw_head[k];
  }
};
