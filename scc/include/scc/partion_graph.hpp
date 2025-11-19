#pragma once

#include <scc/degree.hpp>
#include <scc/graph.hpp>
#include <scc/part.hpp>

/// from a global_graph get the degree of a partition
template<WorldPartConcept Part>
  requires(Part::continuous)
index_t
partition_degree(index_t const* fw_head, Part const& part)
{
  if (part.local_n() > 0) {
    return fw_head[part.end] - fw_head[part.begin];
  }
  return 0;
}

/// from a global_graph get the degree of a partition
template<WorldPartConcept Part>
  requires(not Part::continuous)
index_t
partition_degree(index_t const* fw_head, Part const& part)
{
  auto const local_n = part.local_n();

  index_t m = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    m += degree(fw_head, part.to_global(k));
  }
  return m;
}

template<WorldPartConcept Part>
auto
partition(index_t m, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, Part const& part) -> LocalGraphPart<Part>
{
  auto const local_n = part.local_n();

  auto partition_direction = [&part, local_n](index_t const* head, vertex_t const* csr, index_t* out_head, vertex_t* out_csr) {
    vertex_t pos = 0;
    for (vertex_t k = 0; k < local_n; ++k) {
      auto const u = part.to_global(k);

      auto const beg = head[u];
      auto const end = head[u + 1];

      out_head[k] = pos;
      for (auto it = beg; it < end; ++it) {
        out_csr[pos] = csr[it];
        ++pos;
      }
    }
    out_head[local_n] = pos;
  };

  auto const local_fw_m = partition_degree(fw_head, part);
  auto const local_bw_m = partition_degree(bw_head, part);

  LocalGraphPart<Part> result;
  result.buffer = Buffer::create(
    page_ceil<index_t>(local_n + 1),
    page_ceil<vertex_t>(local_fw_m),
    page_ceil<index_t>(local_n + 1),
    page_ceil<vertex_t>(local_bw_m));
  auto* memory = result.buffer.data();

  result.part       = part;
  result.m          = m;
  result.local_fw_m = local_fw_m;
  result.local_bw_m = local_bw_m;

  result.fw_head = borrow<index_t>(memory, local_n + 1);
  result.fw_csr  = borrow<vertex_t>(memory, local_fw_m);
  partition_direction(fw_head, fw_csr, result.fw_head, result.fw_csr);

  result.bw_head = borrow<index_t>(memory, local_n + 1);
  result.bw_csr  = borrow<vertex_t>(memory, local_bw_m);
  partition_direction(bw_head, bw_csr, result.bw_head, result.bw_csr);

  return result;
}
