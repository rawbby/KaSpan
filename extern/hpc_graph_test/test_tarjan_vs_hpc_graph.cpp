#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/allgather_graph.hpp>
#include <kaspan/scc/scc.hpp>
#include <kaspan/test/fuzzy.hpp>
#include <kaspan/test/normalise_scc.hpp>
#include <kaspan/scc/tarjan.hpp>

#include <adapter/base.hpp>
#include <adapter/kagen.hpp>
#include <fast_map.h>
#include <scc.h>

#include <mpi.h>

#include <cstdio>
#include <string>

int  procid, nprocs;
bool verbose = false;
bool debug   = false;
bool verify  = false;
bool output  = false;

void
test(
  char const* generator_args)
{
  using namespace kaspan;

  auto hpc = kagen_hpc_graph_part(generator_args);
  hpc.initialize();
  auto const part = explicit_sorted_part{ static_cast<vertex_t>(hpc.g.n), static_cast<vertex_t>(hpc.g.n_offset + hpc.g.n_local) };

  ASSERT_EQ(hpc.g.n_local, part.local_n());
  for (u64 k = 0; k < hpc.g.n_local; ++k) {
    auto const u = hpc.g.local_unmap[k];
    ASSERT_EQ(get_value(&hpc.g.map, u), k);
    ASSERT(part.has_local(static_cast<vertex_t>(u)));
    ASSERT_EQ(part.to_local(static_cast<vertex_t>(u)), k);
    ASSERT_EQ(part.to_global(static_cast<vertex_t>(k)), u);
  }
  for (u64 k = 0; k < hpc.g.n_ghost; ++k) {
    auto const u = hpc.g.ghost_unmap[k];
    ASSERT_EQ(get_value(&hpc.g.map, u), k + hpc.g.n_local);
    ASSERT(!part.has_local(static_cast<vertex_t>(u)));
  }

  auto scc_id = make_array<u64>(hpc.g.n_total);
  scc_dist(&hpc.g, &hpc.comm, &hpc.q, hpc.g.max_degree_vert, scc_id.data());
  for (u64 k = 0; k < hpc.g.n_local; ++k) {
    ASSERT_IN_RANGE(scc_id[k], 0, hpc.g.n);
  }
  normalise_scc_id(part.view(), scc_id.data());

  auto const bgp   = kagen_graph_part(generator_args);
  auto const m     = mpi_basic::allreduce_single(bgp.local_fw_m, mpi_basic::sum);
  auto const graph = allgather_graph(m, bgp.fw_view());
  graph.debug_validate();

  ASSERT_EQ(hpc.g.m_local_out, bgp.local_fw_m);
  ASSERT_EQ(hpc.g.m_local_in, bgp.local_bw_m);
  ASSERT_EQ(hpc.g.n, bgp.part.n());
  ASSERT_EQ(hpc.g.n_local, bgp.part.local_n());
  for (u64 k = 0; k <= hpc.g.n_local; ++k) {
    ASSERT_EQ(hpc.g.out_degree_list[k], bgp.fw.head[k], "k={}", k);
    ASSERT_EQ(hpc.g.in_degree_list[k], bgp.bw.head[k], "k={}", k);
  }
  for (u64 i = 0; i < hpc.g.m_local_out; ++i) {
    auto const k = hpc.g.out_edges[i];
    auto const u = k < hpc.g.n_local ? hpc.g.local_unmap[k] : hpc.g.ghost_unmap[k - hpc.g.n_local];
    ASSERT_EQ(u, bgp.fw.csr[i], "i={}", i);
  }
  for (u64 i = 0; i < hpc.g.m_local_in; ++i) {
    auto const k = hpc.g.in_edges[i];
    auto const u = k < hpc.g.n_local ? hpc.g.local_unmap[k] : hpc.g.ghost_unmap[k - hpc.g.n_local];
    ASSERT_EQ(u, bgp.bw.csr[i], "i={}", i);
  }

  tarjan(graph.fw_view(), [&](auto* beg, auto* end) {
    auto const min_u = *std::min_element(beg, end);
    for (auto it = beg; it < end; ++it) {
      auto const u = *it;
      if (part.has_local(u)) {
        auto const k = part.to_local(u);
        ASSERT_EQ(scc_id[k], min_u, "k={}", k);
      }
    }
  });
}

int
main(
  int    argc,
  char** argv)
{
  using namespace kaspan;
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();
  procid = mpi_basic::world_rank;
  nprocs = mpi_basic::world_size;

  auto rng  = std::mt19937{ std::random_device{}() };
  auto dist = std::uniform_int_distribution<u32>();

  auto const kagen_args = std::format("rmat;directed;N=14;M=17;a=0.45;b=0.22;c=0.22;seed={}", dist(rng));
  test(kagen_args.c_str());
}
