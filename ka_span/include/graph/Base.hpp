#pragma once

#include <buffer/Buffer.hpp>
#include <comm/MpiBasic.hpp>
#include <graph/Manifest.hpp>
#include <graph/Partition.hpp>
#include <util/Arithmetic.hpp>

#ifdef KASPAN_32
using index_t  = u32;
using vertex_t = u32;
#else
using index_t  = u64;
using vertex_t = u64;
#endif

constexpr inline auto mpi_index_t  = mpi_basic_type<index_t>;
constexpr inline auto mpi_vertex_t = mpi_basic_type<vertex_t>;

#ifdef KASPAN_COMPRESS_BUFFER
using VertexBuffer = CompressedUnsignedBuffer<index_t>;
using IndexBuffer  = CompressedUnsignedBuffer<vertex_t>;
#else
using VertexBuffer = IntBuffer<index_t>;
using IndexBuffer  = IntBuffer<vertex_t>;
#endif

inline auto
create_head_buffer(vertex_t n, index_t m) -> Result<VertexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(n + 1, needed_bytes(m));
#else
  return VertexBuffer::create(n + 1);
#endif
}

inline auto
create_csr_buffer(vertex_t n, index_t m) -> Result<IndexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  auto const max_vertex = n == 0 ? 0 : n - 1;
  return IndexBuffer::create(m, needed_bytes(max_vertex));
#else
  return IndexBuffer::create(m);
#endif
}

inline auto
create_head_buffer(Manifest const& manifest) -> Result<VertexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(manifest.graph_node_count + 1, manifest.graph_head_bytes);
#else
  return VertexBuffer::create(manifest.graph_node_count + 1);
#endif
}

inline auto
create_csr_buffer(Manifest const& manifest) -> Result<IndexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return IndexBuffer::create(manifest.graph_edge_count, manifest.graph_csr_bytes);
#else
  return IndexBuffer::create(manifest.graph_edge_count);
#endif
}

template<WorldPartitionConcept Partition>
auto
create_head_buffer(Partition const& partition, index_t local_m) -> Result<VertexBuffer> // NOLINT(*-unused-parameters)
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(partition.size() + 1, needed_bytes(local_m));
#else
  return VertexBuffer::create(partition.size() + 1);
#endif
}

template<WorldPartitionConcept Partition>
auto
create_csr_buffer(Partition const& partition, index_t local_m) -> Result<IndexBuffer> // NOLINT(*-unused-parameters)
{
#ifdef KASPAN_COMPRESS_BUFFER
  auto const max_vertex = partition.n == 0 ? 0 : partition.n - 1;
  return IndexBuffer::create(local_m, needed_bytes(max_vertex));
#else
  return IndexBuffer::create(local_m);
#endif
}
