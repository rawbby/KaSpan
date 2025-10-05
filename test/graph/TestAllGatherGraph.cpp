#include <graph/AllGatherGraph.hpp>
#include <graph/convert/Graph2GraphPart.hpp>
#include <scc/Fuzzy.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>

template<WorldPartConcept Part>
void
test_all_gather_graph(kamping::Communicator<>& comm, Graph& graph, Part part)
{
  ASSERT_TRY(auto local_graph, load_local(graph, part));
  ASSERT_TRY(auto const global_graph, AllGatherGraph(comm, local_graph));

  for (vertex_t u = 0; u < graph.n; ++u) {
    ASSERT_EQ(graph.fw_head[u], global_graph.fw_head[u]);
    ASSERT_EQ(graph.bw_head[u], global_graph.bw_head[u]);
  }
  ASSERT_EQ(graph.fw_head[graph.n], global_graph.fw_head[graph.n]);
  ASSERT_EQ(graph.bw_head[graph.n], global_graph.bw_head[graph.n]);

  for (index_t i = 0; i < graph.m; ++i) {
    ASSERT_EQ(graph.fw_csr[i], global_graph.fw_csr[i]);
    ASSERT_EQ(graph.bw_csr[i], global_graph.bw_csr[i]);
  }
}

template<WorldPartConcept Part>
void
test_all_gather_graph_part(kamping::Communicator<>& comm, Graph& graph, Part part)
{
  ASSERT_TRY(auto local_graph, load_local(graph, part));
  ASSERT_TRY(auto local_graph_, load_local(graph, part));
  ComplementBackwardsGraph<Part>(local_graph_);

  for (index_t it = 0; it < local_graph.bw_csr.size(); ++it) {
    ASSERT_EQ(local_graph.bw_csr[it], local_graph_.bw_csr[it]);
  }

  for (vertex_t k = 0; k < local_graph.bw_head.size(); ++k) {
    ASSERT_EQ(local_graph.bw_head[k], local_graph_.bw_head[k], "k=%lu", k + 1);
  }
}

int
main(int argc, char** argv)
{
  constexpr int npc = 1;
  constexpr int npv[1]{ 3 };
  mpi_sub_process(argc, argv, npc, npv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  constexpr auto n    = 1024;
  auto           comm = kamping::Communicator{};

  ASSERT_TRY(auto scc_and_graph, fuzzy_global_scc_id_and_graph(1, n, 1));
  auto [scc, graph] = std::move(scc_and_graph);

  test_all_gather_graph(comm, graph, TrivialSlicePart{ n, comm.rank(), comm.size() });
  test_all_gather_graph(comm, graph, BalancedSlicePart{ n, comm.rank(), comm.size() });
  test_all_gather_graph(comm, graph, CyclicPart{ n, comm.rank(), comm.size() });
  test_all_gather_graph(comm, graph, BlockCyclicPart{ n, comm.rank(), comm.size() });

  test_all_gather_graph_part(comm, graph, TrivialSlicePart{ n, comm.rank(), comm.size() });
  test_all_gather_graph_part(comm, graph, BalancedSlicePart{ n, comm.rank(), comm.size() });
  test_all_gather_graph_part(comm, graph, CyclicPart{ n, comm.rank(), comm.size() });
  test_all_gather_graph_part(comm, graph, BlockCyclicPart{ n, comm.rank(), comm.size() });
}
