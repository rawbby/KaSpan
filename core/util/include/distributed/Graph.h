#pragma once

#include <ErrorCode.hpp>
#include <Util.hpp>
#include <distributed/Buffer.hpp>
#include <distributed/Manifest.hpp>
#include <distributed/Partition.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>

namespace distributed {

template<PartitionConcept Partition = TrivialSlicePartition>
struct Graph
{
  Partition     partition;
  std::uint64_t node_count;
  std::uint64_t edge_count;

  Buffer fw_head;
  Buffer bw_head;
  Buffer fw_csr;
  Buffer bw_csr;

  static Result<Graph> create(const Manifest& manifest, const Partition& part)
  {
    Graph g;
    g.partition  = part;
    g.node_count = manifest.graph_node_count;
    g.edge_count = manifest.graph_edge_count;

    auto load_dir = [&](const std::filesystem::path& head_path, const std::filesystem::path& csr_path, Buffer& out_head, Buffer& out_csr) -> VoidResult {
      RESULT_TRY(const auto head_fb, FileBuffer::create(head_path.c_str(), manifest.graph_head_bytes, g.node_count + 1));
      RESULT_TRY(const auto csr_fb, FileBuffer::create(csr_path.c_str(), manifest.graph_csr_bytes, g.edge_count));

      const std::uint64_t local_n = part.size();
      std::uint64_t       local_m = 0;

      if constexpr (Partition::continuous) {
        if (local_n > 0) {
          const auto begin = head_fb.template get<std::uint64_t>(part.begin);
          const auto end   = head_fb.template get<std::uint64_t>(part.end);
          local_m          = end - begin;
        }
      } else {
        for (std::uint64_t k = 0; k < local_n; ++k) {
          const auto index = part.select(k);
          const auto begin = head_fb.template get<std::uint64_t>(index);
          const auto end   = head_fb.template get<std::uint64_t>(index + 1);
          local_m += end - begin;
        }
      }

      RESULT_TRY(out_head, Buffer::create(manifest.graph_head_bytes, local_n + 1));
      RESULT_TRY(out_csr, Buffer::create(manifest.graph_csr_bytes, local_m));

      std::uint64_t pos = 0;
      for (std::uint64_t k = 0; k < local_n; ++k) {
        const auto index  = part.select(k);
        const auto begin  = head_fb.template get<std::uint64_t>(index);
        const auto end    = head_fb.template get<std::uint64_t>(index + 1);
        const auto degree = end - begin;

        out_head.set(k, pos);

        const auto src = csr_fb.range(begin, end);
        std::memcpy(out_csr[pos].data(), src.data(), src.size_bytes());
        pos += degree;
      }
      out_head.set(local_n, pos);
      return VoidResult::success();
    };

    RESULT_TRY(load_dir(manifest.fw_head_path, manifest.fw_csr_path, g.fw_head, g.fw_csr));
    RESULT_TRY(load_dir(manifest.bw_head_path, manifest.bw_csr_path, g.bw_head, g.bw_csr));

    return g;
  }

  std::uint64_t fw_degree(std::uint64_t index) const
  {
    const auto k     = partition.rank(index);
    const auto begin = fw_head.get<std::uint64_t>(k);
    const auto end   = fw_head.get<std::uint64_t>(k + 1);
    return end - begin;
  }

  std::uint64_t bw_degree(std::uint64_t index) const
  {
    const auto k     = partition.rank(index);
    const auto begin = bw_head.get<std::uint64_t>(k);
    const auto end   = bw_head.get<std::uint64_t>(k + 1);
    return end - begin;
  }

  std::span<const std::byte> fw_adjacency_bytes(std::uint64_t index) const
  {
    const auto k     = partition.rank(index);
    const auto begin = fw_head.get<std::uint64_t>(k);
    const auto end   = fw_head.get<std::uint64_t>(k + 1);
    return fw_csr.range(begin, end);
  }

  std::span<const std::byte> bw_adjacency_bytes(std::uint64_t index) const
  {
    const auto k     = partition.rank(index);
    const auto begin = bw_head.get<std::uint64_t>(k);
    const auto end   = bw_head.get<std::uint64_t>(k + 1);
    return bw_csr.range(begin, end);
  }
};

} // namespace distributed
