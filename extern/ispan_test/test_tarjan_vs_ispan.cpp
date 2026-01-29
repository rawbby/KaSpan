#include "kaspan/test/normalise_scc.hpp"

#include <ispan/scc.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/allgather_graph.hpp>
#include <kaspan/scc/scc.hpp>
#include <kaspan/scc/tarjan.hpp>

#include <mpi.h>

#include <random>

using namespace kaspan;

int
main(
  int    argc,
  char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  auto const bgp = kagen_graph_part("rmat;directed;N=14;M=17;a=0.45;b=0.22;c=0.22");
  bgp.debug_validate();

  auto const m     = mpi_basic::allreduce_single(bgp.local_fw_m, mpi_basic::sum);
  auto const graph = allgather_graph(m, bgp.fw_view());
  graph.debug_validate();

  std::vector<vertex_t> ispan_scc_id;
  scc(graph.n, graph.m, graph.fw.head, graph.fw.csr, graph.bw.head, graph.bw.csr, 12, &ispan_scc_id);
  normalise_scc_id(static_cast<vertex_t>(ispan_scc_id.size()), ispan_scc_id.data());

  tarjan(graph.fw_view(), [&](auto* beg, auto* end) {
    vertex_t min_u = graph.n;
    for (auto it = beg; it < end; ++it) {
      ASSERT_IN_RANGE(*it, 0, graph.n);
      min_u = std::min(min_u, *it);
    }
    for (auto it = beg; it < end; ++it) {
      ASSERT_EQ(ispan_scc_id[*it], min_u);
    }
  });
}
