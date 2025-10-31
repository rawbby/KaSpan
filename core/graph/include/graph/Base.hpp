#pragma once

#include <buffer/Buffer.hpp>
#include <comm/MpiBasic.hpp>
#include <graph/Manifest.hpp>
#include <graph/Part.hpp>
#include <util/Arithmetic.hpp>

#ifndef KASPAN_INDEX64
using index_t = u64;
#else
using index_t = u32;
#endif

#ifndef KASPAN_VERTEX64
using vertex_t = u64;
#else
using vertex_t = u32;
#endif

constexpr inline auto mpi_byte_t   = mpi_basic_type<byte>;
constexpr inline auto mpi_index_t  = mpi_basic_type<index_t>;
constexpr inline auto mpi_vertex_t = mpi_basic_type<vertex_t>;

#ifdef KASPAN_COMPRESS_BUFFER
using VertexBuffer = CompressedUnsignedBuffer<index_t>;
using IndexBuffer  = CompressedUnsignedBuffer<vertex_t>;
#else
using VertexBuffer = IntBuffer<vertex_t>;
using IndexBuffer  = IntBuffer<index_t>;
#endif

inline auto
create_head_buffer(vertex_t n, index_t m) -> Result<IndexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(n + 1, needed_bytes(m));
#else
  return VertexBuffer::create(n + 1);
#endif
}

inline auto
create_csr_buffer(vertex_t n, index_t m) -> Result<VertexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  auto const max_vertex = n == 0 ? 0 : n - 1;
  return IndexBuffer::create(m, needed_bytes(max_vertex));
#else
  return IndexBuffer::create(m);
#endif
}

inline auto
create_head_buffer(Manifest const& manifest) -> Result<IndexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(manifest.graph_node_count + 1, manifest.graph_head_bytes);
#else
  return VertexBuffer::create(manifest.graph_node_count + 1);
#endif
}

inline auto
create_csr_buffer(Manifest const& manifest) -> Result<VertexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return IndexBuffer::create(manifest.graph_edge_count, manifest.graph_csr_bytes);
#else
  return IndexBuffer::create(manifest.graph_edge_count);
#endif
}

template<WorldPartConcept Part>
auto
create_head_buffer(Part const& part, index_t local_m) -> Result<IndexBuffer> // NOLINT(*-unused-parameters)
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(part.local_n() + 1, needed_bytes(local_m));
#else
  return VertexBuffer::create(part.local_n() + 1);
#endif
}

template<WorldPartConcept Part>
auto
create_csr_buffer(Part const& part, index_t local_m) -> Result<VertexBuffer> // NOLINT(*-unused-parameters)
{
#ifdef KASPAN_COMPRESS_BUFFER
  auto const max_vertex = part.n == 0 ? 0 : part.n - 1;
  return IndexBuffer::create(local_m, needed_bytes(max_vertex));
#else
  return IndexBuffer::create(local_m);
#endif
}

// provides all local edges from a graph partition
template<WorldPartConcept Part>
auto
for_local_kuv(Part const& part, index_t const* head, vertex_t const* csr)
{
  return [&part, head, csr](auto fn) {
    vertex_t const local_n = part.local_n();
    for (vertex_t k = 0; k < local_n; ++k) {
      auto const u   = part.to_global(k);
      auto const end = head[k + 1];
      for (auto it = head[k]; it < end; ++it) {
        auto const v = csr[it];
        if (part.has_local(v)) {
          fn(k, u, v);
        }
      }
    }
  };
}

// provides all edges from a graph partition
template<WorldPartConcept Part>
auto
for_kuv(Part part, index_t const* head, vertex_t const* csr)
{
  return [&part, head, csr](auto fn) {
    vertex_t const local_n = part.local_n();
    for (vertex_t k = 0; k < local_n; ++k) {
      auto const u   = part.to_global(k);
      auto const end = head[k + 1];
      for (auto it = head[k]; it < end; ++it) {
        fn(k, u, csr[it]);
      }
    }
  };
}

// provides all edges from a complete graph
inline auto
for_uv(vertex_t n, index_t const* head, vertex_t const* csr)
{
  return [n, head, csr](auto fn) {
    for (vertex_t u = 0; u < n; ++u) {
      auto const end = head[u + 1];
      for (auto it = head[u]; it < end; ++it) {
        fn(u, csr[it]);
      }
    }
  };
}

// provides all local edges from a graph partition fw+bw
template<WorldPartConcept Part>
auto
for_local_kuv(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr)
{
  return [&part, fw_head, fw_csr, bw_head, bw_csr](auto fn) {
    for_local_kuv(part, fw_head, fw_csr)(fn);
    for_local_kuv(part, bw_head, bw_csr)(fn);
  };
}

// provides all edges from a graph partition fw+bw
template<WorldPartConcept Part>
auto
for_kuv(Part part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr)
{
  return [&part, fw_head, fw_csr, bw_head, bw_csr](auto fn) {
    for_kuv(part, fw_head, fw_csr)(fn);
    for_kuv(part, bw_head, bw_csr)(fn);
  };
}

// provides all edges from a complete graph fw+bw
inline auto
for_uv(vertex_t n, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr)
{
  return [n, fw_head, fw_csr, bw_head, bw_csr](auto fn) {
    for_uv(n, fw_head, fw_csr)(fn);
    for_uv(n, bw_head, bw_csr)(fn);
  };
}
