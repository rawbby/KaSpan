#include <debug/statistic.hpp>
#include <debug/sub_process.hpp>
#include <ispan/scc.hpp>
#include <memory/accessor/stack_accessor.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/adapter/manifest.hpp>
#include <scc/allgather_graph.hpp>
#include <scc/base.hpp>
#include <scc/scc.hpp>
#include <mpi_basic/mpi_basic.hpp>

#include <mpi.h>

#include <fstream>
#include <random>

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  auto const graph_part = kagen_graph_part("rmat;directed;N=16;M=18;a=0.25;b=0.25;c=0.25");
  auto const graph      = allgather_graph(
    graph_part.part,
    graph_part.m,
    graph_part.local_fw_m,
    graph_part.fw_head,
    graph_part.fw_csr);

  std::vector<vertex_t> ispan_scc_id;
  scc(
    graph.n,
    graph.m,
    graph.fw_head,
    graph.fw_csr,
    graph.bw_head,
    graph.bw_csr,
    12,
    &ispan_scc_id);

  tarjan(graph.n, graph.fw_head, graph.fw_csr, [&](auto* beg, auto* end) {
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
