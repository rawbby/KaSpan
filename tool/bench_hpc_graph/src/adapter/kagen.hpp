#pragma once

#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <adapter/base.hpp>

inline auto
kagen_hpc_graph_part(
  char const* generator_args) -> hpc_graph_data
{
  using namespace kaspan;
  auto const g = kagen_graph_part(generator_args);

  hpc_graph_data data{};
  data.g.n           = integral_cast<u64>(g.part.n());
  data.g.m_local_out = integral_cast<u64>(g.local_fw_m);
  data.g.m_local_in  = integral_cast<u64>(g.local_bw_m);
  data.g.n_local     = integral_cast<u64>(g.part.local_n());
  data.g.n_offset    = integral_cast<u64>(g.part.begin());
  data.g.out_edges   = line_alloc<u64>(data.g.m_local_out);
  data.g.in_edges    = line_alloc<u64>(data.g.m_local_in);

  if (data.g.n_local > 0) {
    data.g.out_degree_list = line_alloc<u64>(data.g.n_local + 1);
    data.g.in_degree_list  = line_alloc<u64>(data.g.n_local + 1);
    for (u64 k = 0; k <= data.g.n_local; ++k) {
      data.g.out_degree_list[k] = integral_cast<u64>(g.fw.head[k]);
      data.g.in_degree_list[k]  = integral_cast<u64>(g.bw.head[k]);
    }
  }

  for (u64 i = 0; i < data.g.m_local_out; ++i) {
    data.g.out_edges[i] = integral_cast<u64>(g.fw.csr[i]);
  }
  for (u64 i = 0; i < data.g.m_local_in; ++i) {
    data.g.in_edges[i] = integral_cast<u64>(g.bw.csr[i]);
  }

  data.initialize(); // count into the benchmark
  return data;
}
