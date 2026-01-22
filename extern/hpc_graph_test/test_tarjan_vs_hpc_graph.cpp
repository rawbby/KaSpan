#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/allgather_graph.hpp>
#include <kaspan/scc/scc.hpp>
#include <kaspan/test/fuzzy.hpp>
#include <kaspan/test/normalise_scc.hpp>

#include <adapter/base.hpp>
#include <adapter/kagen.hpp>
#include <scc.h>

#include <mpi.h>

#include <cstdio>
#include <string>

int  procid, nprocs;
bool verbose = false;
bool debug   = false;
bool verify  = false;
bool output  = false;

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

  auto hpc = kagen_hpc_graph_part("rmat;directed;N=16;M=18;a=0.25;b=0.25;c=0.25");
  hpc.initialize();
  auto const part = explicit_sorted_part{ static_cast<vertex_t>(hpc.g.n), static_cast<vertex_t>(hpc.g.n_offset + hpc.g.n_local) };

  auto scc_id = make_array<u64>(hpc.g.n_total);
  scc_dist(&hpc.g, &hpc.comm, &hpc.q, 0, scc_id.data());
  normalise_scc_id(part.view(), scc_id.data());

  auto const bgp   = kagen_graph_part("rmat;directed;N=16;M=18;a=0.25;b=0.25;c=0.25");
  auto const m     = mpi_basic::allreduce_single(bgp.local_fw_m, mpi_basic::sum);
  auto const graph = allgather_graph(m, bgp.fw_view());

  tarjan(graph.fw_view(), [&](auto* beg, auto* end) {
    auto const min_u = *std::min_element(beg, end);
    for (auto it = beg; it < end; ++it)
      ASSERT_EQ(scc_id[*it], min_u);
  });
}
