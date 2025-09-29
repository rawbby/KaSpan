#pragma once

#include <buffer/Buffer.hpp>
#include <buffer/Copy.hpp>
#include <graph/Base.hpp>
#include <graph/Manifest.hpp>
#include <util/Arithmetic.hpp>

struct Graph
{
  vertex_t n = 0;
  index_t  m = 0;

  VertexBuffer fw_head;
  IndexBuffer  fw_csr;
  VertexBuffer bw_head;
  IndexBuffer  bw_csr;

  Graph()  = default;
  ~Graph() = default;

  Graph(Graph const&) = delete;
  Graph(Graph&& rhs) noexcept
    : n(rhs.n)
    , m(rhs.m)
    , fw_head(std::move(rhs.fw_head))
    , fw_csr(std::move(rhs.fw_csr))
    , bw_head(std::move(rhs.bw_head))
    , bw_csr(std::move(rhs.bw_csr))
  {
  }

  auto operator=(Graph const&) -> Graph& = delete;
  auto operator=(Graph&& rhs) noexcept -> Graph&
  {
    if (this != &rhs) {
      n       = rhs.n;
      m       = rhs.m;
      fw_head = std::move(rhs.fw_head);
      fw_csr  = std::move(rhs.fw_csr);
      bw_head = std::move(rhs.bw_head);
      bw_csr  = std::move(rhs.bw_csr);
    }
    return *this;
  }

  static auto load(Manifest const& manifest) -> Result<Graph>
  {
    using LoadBuffer = CU64Buffer<FileBuffer>;

    auto const  n            = manifest.graph_node_count;
    auto const  m            = manifest.graph_edge_count;
    auto const  head_bytes   = manifest.graph_head_bytes;
    auto const  csr_bytes    = manifest.graph_csr_bytes;
    auto const  load_endian  = manifest.graph_endian;
    char const* fw_head_file = manifest.fw_head_path.c_str();
    char const* fw_csr_file  = manifest.fw_csr_path.c_str();
    char const* bw_head_file = manifest.bw_head_path.c_str();
    char const* bw_csr_file  = manifest.bw_csr_path.c_str();

    RESULT_ASSERT(n < std::numeric_limits<vertex_t>::max(), ASSUMPTION_ERROR);
    RESULT_ASSERT(m < std::numeric_limits<index_t>::max(), ASSUMPTION_ERROR);

    RESULT_TRY(auto const fw_head, LoadBuffer::create_r(fw_head_file, n + 1, head_bytes, load_endian));
    RESULT_TRY(auto const fw_csr, LoadBuffer::create_r(fw_csr_file, m, csr_bytes, load_endian));
    RESULT_TRY(auto const bw_head, LoadBuffer::create_r(bw_head_file, n + 1, head_bytes, load_endian));
    RESULT_TRY(auto const bw_csr, LoadBuffer::create_r(bw_csr_file, m, csr_bytes, load_endian));

    auto g = Graph{};
    g.n    = n;
    g.m    = m;

    RESULT_TRY(g.fw_head, create_head_buffer(manifest));
    copy(g.fw_head, fw_head);

    RESULT_TRY(g.fw_csr, create_csr_buffer(manifest));
    copy(g.fw_csr, fw_csr);

    RESULT_TRY(g.bw_head, create_head_buffer(manifest));
    copy(g.bw_head, bw_head);

    RESULT_TRY(g.bw_csr, create_csr_buffer(manifest));
    copy(g.bw_csr, bw_csr);

    return g;
  }

  [[nodiscard]] auto fw_degree(index_t k) const -> index_t
  {
    return fw_head[k + 1] - fw_head[k];
  }

  [[nodiscard]] auto bw_degree(index_t k) const -> index_t
  {
    return fw_head[k + 1] - fw_head[k];
  }
};
