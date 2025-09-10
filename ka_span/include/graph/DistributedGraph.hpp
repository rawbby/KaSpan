#pragma once

#include <buffer/Buffer.hpp>
#include <comm/SyncEdgeComm.hpp>
#include <graph/Manifest.hpp>
#include <graph/Partition.hpp>
#include <util/Arithmetic.hpp>
#include <util/Result.hpp>

#include <cstring>
#include <filesystem>
#include <kagen.h>

template<PartitionConcept Partition = TrivialSlicePartition, bool Compress = false>
struct DistributedGraph
{
  using buffer_type = std::conditional_t<Compress, CU64Buffer<Buffer>, U64Buffer>;

  Partition partition;
  u64       n;
  u64       m;

  buffer_type fw_head;
  buffer_type bw_head;
  buffer_type fw_csr;
  buffer_type bw_csr;

  DistributedGraph(Partition partition, u64 n, u64 m)
    : partition(partition)
    , n(n)
    , m(m)
  {
  }

  static auto load(Partition const& partition, Manifest const& manifest) -> Result<DistributedGraph>
  {
    auto const n      = manifest.graph_node_count;
    auto const m      = manifest.graph_edge_count;
    auto const endian = manifest.graph_endian;

    auto load_dir = [&](std::filesystem::path const& head_path, std::filesystem::path const& csr_path, auto& out_head, auto& out_csr) -> VoidResult {
      RESULT_TRY(auto const head_fb, (CU64Buffer<FileBuffer>::create_r(head_path.c_str(), n + 1, manifest.graph_head_bytes, endian)));
      RESULT_TRY(auto const csr_fb, (CU64Buffer<FileBuffer>::create_r(csr_path.c_str(), m, manifest.graph_csr_bytes, endian)));

      u64 const local_n = partition.size();
      u64       local_m = 0;

      if constexpr (Partition::continuous) {
        if (local_n > 0) {
          auto const begin = head_fb.get(partition.begin);
          auto const end   = head_fb.get(partition.end);
          local_m          = end - begin;
        }
      } else {
        for (u64 k = 0; k < local_n; ++k) {
          auto const index = partition.select(k);
          auto const begin = head_fb.get(index);
          auto const end   = head_fb.get(index + 1);
          local_m += end - begin;
        }
      }

      if constexpr (Compress) {
        RESULT_TRY(out_head, buffer_type::create(local_n + 1, manifest.graph_head_bytes));
        RESULT_TRY(out_csr, buffer_type::create(local_m, manifest.graph_csr_bytes));
      } else {
        RESULT_TRY(out_head, buffer_type::create(local_n + 1));
        RESULT_TRY(out_csr, buffer_type::create(local_m));
      }

      u64 pos = 0;
      for (u64 k = 0; k < local_n; ++k) {
        auto const index = partition.select(k);
        auto const begin = head_fb.get(index);
        auto const end   = head_fb.get(index + 1);

        out_head.set(k, pos);

        for (auto it = begin; it != end; ++it)
          out_csr.set(pos++, csr_fb.get(it));
      }
      out_head.set(local_n, pos);
      return VoidResult::success();
    };

    buffer_type fw_head;
    buffer_type bw_head;
    buffer_type fw_csr;
    buffer_type bw_csr;

    RESULT_TRY(load_dir(manifest.fw_head_path, manifest.fw_csr_path, fw_head, fw_csr));
    RESULT_TRY(load_dir(manifest.bw_head_path, manifest.bw_csr_path, bw_head, bw_csr));

    DistributedGraph result{ partition, n, m };
    result.fw_head = std::move(fw_head);
    result.bw_head = std::move(bw_head);
    result.fw_csr  = std::move(fw_csr);
    result.bw_csr  = std::move(bw_csr);
    return result;
  }

  // static auto generate(kamping::Communicator<>& comm, std::string const& optString) -> Result<DistributedGraph>
  // {
  //   kagen::KaGen gen(MPI_COMM_WORLD);
  //   gen.UseCSRRepresentation();
  //   auto const kg_graph               = gen.GenerateFromOptionString(optString);
  //   auto const n                      = kg_graph.NumberOfGlobalVertices();
  //   auto const m                      = kg_graph.NumberOfGlobalEdges();
  //   auto const local_n                = kg_graph.xadj.size() - 1;
  //   auto const local_m                = kg_graph.adjncy.size();
  //   auto const [node_begin, node_end] = kg_graph.vertex_range;
  //   ASSERT_TRY(node_end - node_begin == local_n, ASSUMPTION_ERROR);
  //   RESULT_TRY(auto part, KaGenPartition::create(n, node_begin, node_end, comm.rank(), comm.size()));
  //   auto const needed_head_bytes = needed_bytes(local_m);
  //   auto const needed_csr_bytes  = needed_bytes(n - 1);
  //   if constexpr (Compress) {
  //     RESULT_TRY(result.fw_head, buffer_type::create(local_n + 1, needed_head_bytes));
  //     RESULT_TRY(result.fw_csr, buffer_type::create(local_m, needed_csr_bytes));
  //     RESULT_TRY(result.bw_head, buffer_type::create(local_n + 1, needed_head_bytes));
  //   } else {
  //     RESULT_TRY(result.fw_head, buffer_type::create(local_n + 1));
  //     RESULT_TRY(result.fw_csr, buffer_type::create(local_m));
  //     RESULT_TRY(result.bw_head, buffer_type::create(local_n + 1));
  //   }
  //   for (u64 i = 0; i < local_n + 1; ++i)
  //     result.fw_head.set(i, static_cast<u64>(kg_graph.xadj[i]));
  //   for (u64 i = 0; i < local_m; ++i)
  //     result.fw_csr.set(i, static_cast<u64>(kg_graph.adjncy[i]));
  //   using payload_type = MpiTupleType<std::uint64_t, std::uint64_t>;
  //   RESULT_TRY(auto edge_comm, SyncEdgeComm<Partition>::create(&comm));
  //   for (size_t i = 0; i < result.partition.n; ++i) {
  //     auto const u     = result.partition.select(n);
  //     auto const begin = result.fw_head.get(i);
  //     auto const end   = result.fw_head.get(i + 1);
  //     for (auto j = begin; j < end; ++j) {
  //       auto const v = result.fw_csr.get(j);
  //       edge_comm.push_relaxed(comm, part, { v, u });
  //     }
  //   }
  //   edge_comm.template communicate<false>(comm, part);
  //   auto massages = edge_comm.messages();
  //   DistributedGraph result{ std::move(part), n, m };
  //   return Result<DistributedGraph>::success(std::move(result));
  // }

  [[nodiscard]] auto fw_degree(u64 k) const -> u64
  {
    return fw_head.get(k + 1) - fw_head.get(k);
  }

  [[nodiscard]] auto bw_degree(u64 k) const -> u64
  {
    return bw_head.get(k + 1) - bw_head.get(k);
  }
};
