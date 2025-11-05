#pragma once

#include <scc/graph.hpp>
#include <scc/part.hpp>
#include <util/arithmetic.hpp>

template<WorldPartConcept Part>
auto
partition(Graph& g, Part const& part)
{
  struct Result
  {
    Buffer          buffer;
    GraphPart<Part> graph;
  };

  auto get_local_m = [&part](index_t* head) -> index_t {
    vertex_t const n = part.local_n();
    index_t        m = 0;
    if constexpr (Part::continuous) {
      if (n > 0)
        m = head[part.end] - head[part.begin];
    } else {
      for (vertex_t k = 0; k < n; ++k) {
        auto const u = part.to_global(k);
        m += head[u + 1] - head[u];
      }
    }
    return m;
  };

  auto const local_n    = part.local_n();
  auto const local_fw_m = get_local_m(g.fw_head);
  auto const local_bw_m = get_local_m(g.bw_head);

  auto load_dir = [&part, &local_n](index_t const* head, vertex_t const* csr, index_t* out_head, vertex_t* out_csr) {
    u64 pos = 0;
    for (u64 k = 0; k < local_n; ++k) {
      auto const u = part.to_global(k);

      out_head[k] = pos;
      for (auto v = head[u]; v < head[u + 1]; ++v)
        out_csr[pos++] = csr[v];
    }
    out_head[local_n] = pos;
  };

  Result result;
  result.buffer = Buffer::create(2 * page_ceil((local_n + 1) * sizeof(index_t)) + page_ceil(local_fw_m * sizeof(vertex_t)) + page_ceil(local_bw_m * sizeof(vertex_t)));
  auto* access  = result.buffer.data();

  result.graph.part = part;
  result.graph.n    = g.n;
  result.graph.m    = g.m;

  result.graph.fw_head = ::borrow<index_t>(access, local_n + 1);
  result.graph.fw_csr  = ::borrow<vertex_t>(access, local_fw_m);
  load_dir(g.fw_head, g.fw_csr, result.graph.fw_head, result.graph.fw_csr);

  result.graph.bw_head = ::borrow<index_t>(access, local_n + 1);
  result.graph.bw_csr  = ::borrow<vertex_t>(access, local_bw_m);
  load_dir(g.bw_head, g.bw_csr, result.graph.bw_head, result.graph.bw_csr);

  return result;
}

template<WorldPartConcept Part>
auto
partition(Graph& g, vertex_t const* scc_id, Part const& part)
{
  struct Result
  {
    Buffer          buffer;
    GraphPart<Part> graph;
    vertex_t*       scc_id;
  };

  auto get_local_m = [&part](index_t* head) -> index_t {
    vertex_t const n = part.local_n();
    index_t        m = 0;
    if constexpr (Part::continuous) {
      if (n > 0)
        m = head[part.end] - head[part.begin];
    } else {
      for (vertex_t k = 0; k < n; ++k) {
        auto const u = part.to_global(k);
        m += head[u + 1] - head[u];
      }
    }
    return m;
  };

  auto const local_n    = part.local_n();
  auto const local_fw_m = get_local_m(g.fw_head);
  auto const local_bw_m = get_local_m(g.bw_head);

  auto load_dir = [&part, &local_n](index_t const* head, vertex_t const* csr, index_t* out_head, vertex_t* out_csr) {
    u64 pos = 0;
    for (u64 k = 0; k < local_n; ++k) {
      auto const u = part.to_global(k);

      out_head[k] = pos;
      for (auto v = head[u]; v < head[u + 1]; ++v)
        out_csr[pos++] = csr[v];
    }
    out_head[local_n] = pos;
  };

  Result result;
  result.buffer = Buffer::create(page_ceil(local_n * sizeof(vertex_t)) + 2 * page_ceil((local_n + 1) * sizeof(index_t)) + page_ceil(local_fw_m * sizeof(vertex_t)) + page_ceil(local_bw_m * sizeof(vertex_t)));
  auto* access  = result.buffer.data();

  result.scc_id = ::borrow<vertex_t>(access, local_n);
  for (size_t k = 0; k < local_n; ++k)
    result.scc_id[k] = scc_id[part.to_global(k)];

  result.graph.part = part;
  result.graph.n    = g.n;
  result.graph.m    = g.m;

  result.graph.fw_head = ::borrow<index_t>(access, local_n + 1);
  result.graph.fw_csr  = ::borrow<vertex_t>(access, local_fw_m);
  load_dir(g.fw_head, g.fw_csr, result.fw_head, result.fw_csr);

  result.graph.bw_head = ::borrow<index_t>(access, local_n + 1);
  result.graph.bw_csr  = ::borrow<vertex_t>(access, local_bw_m);
  load_dir(g.bw_head, g.bw_csr, result.bw_head, result.bw_csr);

  return result;
}
