#pragma once

#include <kaspan/scc/degree.hpp>
#include <kaspan/scc/graph.hpp>
#include <kaspan/scc/part.hpp>

/// from a global_graph get the degree of a partition

namespace kaspan {

template<world_part_concept part_t>
  requires(part_t::continuous)
index_t
partition_degree(index_t const* fw_head, part_t const& part)
{
  if (part.local_n() > 0) { return fw_head[part.end] - fw_head[part.begin]; }
  return 0;
}

/// from a global_graph get the degree of a partition
template<world_part_concept part_t>
  requires(not part_t::continuous)
index_t
partition_degree(index_t const* fw_head, part_t const& part)
{
  auto const local_n = part.local_n();

  index_t m = 0;
  for (vertex_t k = 0; k < local_n; ++k) { m += degree(fw_head, part.to_global(k)); }
  return m;
}

template<world_part_concept part_t>
auto
partition(index_t m, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, part_t const& part) -> local_graph_part<part_t>
{
  auto const local_n = part.local_n();
  DEBUG_ASSERT_VALID_GRAPH(part.n, m, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(part.n, m, bw_head, bw_csr);

  auto partition_direction = [&part, local_n, m](index_t const* head, vertex_t const* csr, index_t* out_head, vertex_t* out_csr) {
    DEBUG_ASSERT_VALID_GRAPH(part.n, m, head, csr);

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

  local_graph_part<part_t> result;
  result.storage = make_graph_buffer(local_n, local_fw_m, local_bw_m);
  auto* memory   = result.storage.data();

  result.part       = part;
  result.m          = m;
  result.local_fw_m = local_fw_m;
  result.local_bw_m = local_bw_m;

  result.fw_head = borrow_array<index_t>(&memory, local_n + 1);
  result.fw_csr  = borrow_array<vertex_t>(&memory, local_fw_m);
  partition_direction(fw_head, fw_csr, result.fw_head, result.fw_csr);

  result.bw_head = borrow_array<index_t>(&memory, local_n + 1);
  result.bw_csr  = borrow_array<vertex_t>(&memory, local_bw_m);
  partition_direction(bw_head, bw_csr, result.bw_head, result.bw_csr);

  return result;
}

template<world_part_concept part_t>
auto
partition(index_t m, index_t const* head, vertex_t const* csr, part_t const& part)
{
  struct partition_result
  {
    buffer    storage;
    part_t    part;
    index_t   m       = 0;
    index_t   local_m = 0;
    index_t*  head    = nullptr;
    vertex_t* csr     = nullptr;
  };

  auto const local_n = part.local_n();
  auto const local_m = partition_degree(head, part);

  partition_result result;
  result.storage = make_fw_graph_buffer(local_n + 1, local_m);
  auto* memory   = result.storage.data();

  result.part    = part;
  result.m       = m;
  result.local_m = local_m;

  result.head  = borrow_array<index_t>(&memory, local_n + 1);
  result.csr   = borrow_array<vertex_t>(&memory, local_m);
  vertex_t pos = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u = part.to_global(k);

    auto const beg = head[u];
    auto const end = head[u + 1];

    result.head[k] = pos;
    for (auto it = beg; it < end; ++it) {
      result.csr[pos] = csr[it];
      ++pos;
    }
  }
  result.head[local_n] = pos;

  return result;
}

} // namespace kaspan
