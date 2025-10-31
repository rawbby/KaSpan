#pragma once

#include <util/Arithmetic.hpp>
#include <util/Result.hpp>
#include <util/Util.hpp>

#include <graph/Graph.hpp>
#include <graph/GraphPart.hpp>
#include <graph/Part.hpp>

#include <utility>

template<WorldPartConcept Part>
auto
load_local(Graph& g, Part part) -> Result<GraphPart<Part>>
{
  using DG = GraphPart<Part>;

  auto load_dir = [&](auto const& head, auto const& csr, VertexBuffer& out_head, IndexBuffer& out_csr) -> VoidResult {
    u64 const local_n = part.local_n();
    u64       local_m = 0;
    if constexpr (Part::continuous) {
      if (local_n > 0)
        local_m = head[part.end] - head[part.begin];
    } else {
      for (u64 k = 0; k < local_n; ++k) {
        auto const i = part.to_global(k);
        local_m += head[i + 1] - head[i];
      }
    }

    RESULT_TRY(out_head, create_head_buffer(part, local_m));
    RESULT_TRY(out_csr, create_csr_buffer(part, local_m));

    u64 pos = 0;
    for (u64 k = 0; k < local_n; ++k) {
      auto const u = part.to_global(k);

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
  result.part = std::move(part);
  result.n    = g.n;
  result.m    = g.m;
  return result;
}
