#include <debug/statistic.hpp>
#include <debug/sub_process.hpp>
#include <ispan/scc.hpp>
#include <memory/stack_accessor.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/adapter/manifest.hpp>
#include <scc/allgather_graph.hpp>
#include <scc/base.hpp>
#include <scc/scc.hpp>
#include <util/mpi_basic.hpp>

#include <mpi.h>

#include <fstream>
#include <random>

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  auto const graph_part = kagen_graph_part("rmat;directed;N=16;M=18;a=0.25;b=0.25;c=0.25");
  DEBUG_ASSERT_VALID_GRAPH_PART(graph_part.part, graph_part.fw_head, graph_part.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(graph_part.part, graph_part.bw_head, graph_part.bw_csr);

  auto const graph      = allgather_graph(
    graph_part.part,
    graph_part.m,
    graph_part.local_fw_m,
    graph_part.fw_head,
    graph_part.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(graph.n, graph.m, graph.fw_head, graph.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(graph.n, graph.m, graph.bw_head, graph.bw_csr);

  auto  scc_id_buffer = Buffer{ 2 * page_ceil<vertex_t>(graph_part.part.local_n()) };
  auto* scc_id_access = scc_id_buffer.data();
  auto* kaspan_scc_id = borrow_clean<vertex_t>(scc_id_access, graph_part.part.local_n());
  scc(
    graph_part.part,
    graph_part.fw_head,
    graph_part.fw_csr,
    graph_part.bw_head,
    graph_part.bw_csr,
    kaspan_scc_id);

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

  ASSERT_EQ(ispan_scc_id.size(), graph_part.part.n);
  for (vertex_t k = 0; k < graph_part.part.local_n(); ++k) {
    auto const u = graph_part.part.to_global(k);
    ASSERT_EQ(kaspan_scc_id[k], ispan_scc_id[u]);
  }
}
