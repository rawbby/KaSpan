#include <ispan/scc.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/allgather_graph.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/scc.hpp>

#include <mpi.h>

using namespace kaspan;

int
main(
  int    argc,
  char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  auto const bgp = kagen_graph_part("rmat;directed;N=16;M=18;a=0.25;b=0.25;c=0.25");
  bgp.debug_validate();

  auto const m     = mpi_basic::allreduce_single(bgp.local_fw_m, mpi_basic::sum);
  auto const graph = allgather_graph(m, bgp.fw_view());
  graph.debug_validate();

  auto const local_n       = bgp.part.local_n();
  auto       kaspan_scc_id = make_array<vertex_t>(local_n);
  scc(bgp.view(), kaspan_scc_id.data());

  std::vector<vertex_t> ispan_scc_id;
  scc(graph.n, graph.m, graph.fw.head, graph.fw.csr, graph.bw.head, graph.bw.csr, 12, &ispan_scc_id);

  ASSERT_EQ(ispan_scc_id.size(), bgp.part.n());
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u = bgp.part.to_global(k);
    ASSERT_EQ(kaspan_scc_id[k], ispan_scc_id[u]);
  }
}
