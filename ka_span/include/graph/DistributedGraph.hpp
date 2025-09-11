#pragma once

#include <buffer/Buffer.hpp>
#include <comm/SyncEdgeComm.hpp>
#include <graph/Base.hpp>
#include <graph/Manifest.hpp>
#include <graph/Partition.hpp>
#include <util/Arithmetic.hpp>
#include <util/Result.hpp>

#include <cstring>
#include <filesystem>
#include <kagen.h>

template<PartitionConcept Partition = TrivialSlicePartition>
struct DistributedGraph
{
  Partition partition;
  vertex_t  n = 0;
  index_t   m = 0;

  VertexBuffer fw_head;
  VertexBuffer bw_head;
  IndexBuffer  fw_csr;
  IndexBuffer  bw_csr;

  DistributedGraph()  = default;
  ~DistributedGraph() = default;

  DistributedGraph(DistributedGraph const&) = delete;
  DistributedGraph(DistributedGraph&& rhs) noexcept
  {
    partition = std::move(rhs.partition);
    n         = rhs.n;
    m         = rhs.m;
    fw_head   = std::move(rhs.fw_head);
    bw_head   = std::move(rhs.bw_head);
    fw_csr    = std::move(rhs.fw_csr);
    bw_csr    = std::move(rhs.bw_csr);
    rhs.n     = 0;
    rhs.m     = 0;
  }

  auto operator=(DistributedGraph const&) -> DistributedGraph& = delete;
  auto operator=(DistributedGraph&& rhs) noexcept -> DistributedGraph&
  {
    if (this != &rhs) {
      partition = std::move(rhs.partition);
      n         = rhs.n;
      m         = rhs.m;
      fw_head   = std::move(rhs.fw_head);
      bw_head   = std::move(rhs.bw_head);
      fw_csr    = std::move(rhs.fw_csr);
      bw_csr    = std::move(rhs.bw_csr);
      rhs.n     = 0;
      rhs.m     = 0;
    }
    return *this;
  }

  static auto load(Partition partition, Manifest const& manifest) -> Result<DistributedGraph>
  {
    using LoadBuffer = CU64Buffer<FileBuffer>;

    auto const n            = manifest.graph_node_count;
    auto const m            = manifest.graph_edge_count;
    auto const head_bytes   = manifest.graph_head_bytes;
    auto const csr_bytes    = manifest.graph_csr_bytes;
    auto const load_endian  = manifest.graph_endian;
    auto const fw_head_file = manifest.fw_head_path.c_str();
    auto const fw_csr_file  = manifest.fw_csr_path.c_str();
    auto const bw_head_file = manifest.bw_head_path.c_str();
    auto const bw_csr_file  = manifest.bw_csr_path.c_str();

    RESULT_ASSERT(n < std::numeric_limits<vertex_t>::max(), ASSUMPTION_ERROR);
    RESULT_ASSERT(m < std::numeric_limits<index_t>::max(), ASSUMPTION_ERROR);

    auto load_dir = [&](char const* head_file, char const* csr_file, auto& graph_head, auto& graph_csr) -> VoidResult {
      RESULT_TRY(auto const head, LoadBuffer::create_r(head_file, n + 1, head_bytes, load_endian));
      RESULT_TRY(auto const csr, LoadBuffer::create_r(csr_file, m, csr_bytes, load_endian));

      vertex_t const local_n = partition.size();
      index_t        local_m = 0;

      if constexpr (Partition::continuous) {
        if (local_n > 0)
          local_m = head.get(partition.end) - head.get(partition.begin);
      } else {
        for (vertex_t k = 0; k < local_n; ++k) {
          auto const u = partition.select(k);
          local_m += head.get(u + 1) - head.get(u);
        }
      }

      RESULT_TRY(graph_head, create_head_buffer(partition, local_m));
      RESULT_TRY(graph_csr, create_csr_buffer(partition, local_m));

      u64 pos = 0;
      for (u64 k = 0; k < local_n; ++k) {
        auto const index = partition.select(k);
        auto const begin = head.get(index);
        auto const end   = head.get(index + 1);

        graph_head.set(k, pos);

        for (auto it = begin; it != end; ++it)
          graph_csr.set(pos++, csr.get(it));
      }
      graph_head.set(local_n, pos);
      return VoidResult::success();
    };

    auto result = DistributedGraph{};
    RESULT_TRY(load_dir(fw_head_file, fw_csr_file, result.fw_head, result.fw_csr));
    RESULT_TRY(load_dir(bw_head_file, bw_csr_file, result.bw_head, result.bw_csr));
    result.partition = std::move(partition);
    result.n         = n;
    result.m         = m;
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
  //   RESULT_ASSERT(node_end - node_begin == local_n, ASSUMPTION_ERROR);
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
