#pragma once

#include <kaspan/memory/accessor/dense_unsigned_accessor.hpp>
#include <kaspan/memory/file_buffer.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <adapter/base.hpp>

template<kaspan::part_view_concept Part>
auto
load_hpc_graph_from_manifest(
  Part                    part,
  kaspan::manifest const& manifest) -> hpc_graph_data
{
  using namespace kaspan;
  DEBUG_ASSERT_EQ(manifest.graph_node_count, part.n());

  auto const  n            = part.n();
  auto const  local_n      = part.local_n();
  auto const  m            = manifest.graph_edge_count;
  auto const  head_bytes   = manifest.graph_head_bytes;
  auto const  csr_bytes    = manifest.graph_csr_bytes;
  auto const  load_endian  = manifest.graph_endian;
  auto const* fw_head_file = manifest.fw_head_path.c_str();
  auto const* fw_csr_file  = manifest.fw_csr_path.c_str();
  auto const* bw_head_file = manifest.bw_head_path.c_str();
  auto const* bw_csr_file  = manifest.bw_csr_path.c_str();

  auto fw_head_buffer = file_buffer::create_r(fw_head_file, (n + 1) * head_bytes);
  auto fw_csr_buffer  = file_buffer::create_r(fw_csr_file, m * csr_bytes);
  auto bw_head_buffer = file_buffer::create_r(bw_head_file, (n + 1) * head_bytes);
  auto bw_csr_buffer  = file_buffer::create_r(bw_csr_file, m * csr_bytes);
  auto fw_head_access = view_dense_unsigned(fw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto fw_csr_access  = view_dense_unsigned(fw_csr_buffer.data(), m, csr_bytes, load_endian);
  auto bw_head_access = view_dense_unsigned(bw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto bw_csr_access  = view_dense_unsigned(bw_csr_buffer.data(), m, csr_bytes, load_endian);

  auto const head_begin = part.begin();
  auto const head_end   = part.end();

  auto const fw_csr_begin = fw_head_access.get(head_begin);
  auto const fw_csr_end   = fw_head_access.get(head_end);
  auto const local_fw_m   = fw_csr_end - fw_csr_begin;

  auto const bw_csr_begin = bw_head_access.get(part.begin());
  auto const bw_csr_end   = bw_head_access.get(part.end());
  auto const local_bw_m   = bw_csr_end - bw_csr_begin;

  hpc_graph_data data{};
  data.g.n           = integral_cast<u64>(n);
  data.g.m_local_out = integral_cast<u64>(local_fw_m);
  data.g.m_local_in  = integral_cast<u64>(local_bw_m);
  data.g.n_local     = integral_cast<u64>(local_n);
  data.g.n_offset    = integral_cast<u64>(head_begin);
  data.g.out_edges   = line_alloc<u64>(local_fw_m);
  data.g.in_edges    = line_alloc<u64>(local_bw_m);

  if (local_n > 0) {
    data.g.out_degree_list = line_alloc<u64>(local_n + 1);
    data.g.in_degree_list  = line_alloc<u64>(local_n + 1);
    for (u64 k = head_begin; k <= head_end; ++k) {
      data.g.out_degree_list[k - head_begin] = integral_cast<u64>(fw_head_access.get(k) - fw_csr_begin);
      data.g.in_degree_list[k - head_begin]  = integral_cast<u64>(bw_head_access.get(k) - bw_csr_begin);
    }
  }

  for (u64 i = fw_csr_begin; i < fw_csr_end; ++i) {
    data.g.out_edges[i - fw_csr_begin] = integral_cast<u64>(fw_csr_access.get(i));
  }
  for (u64 i = bw_csr_begin; i < bw_csr_end; ++i) {
    data.g.in_edges[i - bw_csr_begin] = integral_cast<u64>(bw_csr_access.get(i));
  }

  return data;
}
