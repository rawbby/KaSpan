#include <../../core/essential/include/debug/Assert.hpp>
#include <../../core/essential/include/debug/SubProcess.hpp>
#include <../../core/graph/include/graph/LocalPartioning.hpp>
#include <scc/allgather_graph.hpp>
#include <scc/fuzzy.hpp>
#include <util/scope_guard.hpp>

/// memory: 2 * page_ceil((n + 1) * sizeof(index_t)) + 2 * page_ceil(m * sizeof(vertex_t))
template<WorldPartConcept Part>
void
test_all_gather_graph(Graph& graph, Part part, void * __restrict memory)
{
  ASSERT_TRY(auto local_graph, partition(graph, part));
  ASSERT_TRY(auto const global_graph, AllGatherGraph(local_graph, memory));

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

// template<WorldPartConcept Part>
// void
// test_all_gather_graph_part(Graph& graph, Part part)
// {
//   ASSERT_TRY(auto local_graph, load_local(graph, part));
//   ASSERT_TRY(auto local_graph_, load_local(graph, part));
//   ComplementBackwardsGraph<Part>(local_graph_);
//
//   for (index_t it = 0; it < local_graph.graph.bw_csr.size(); ++it) {
//     ASSERT_EQ(local_graph.bw_csr[it], local_graph_.bw_csr[it]);
//   }
//
//   for (vertex_t k = 0; k < local_graph.bw_head.size(); ++k) {
//     ASSERT_EQ(local_graph.bw_head[k], local_graph_.bw_head[k], "k=%u", k + 1);
//   }
// }

int
main(int argc, char** argv)
{
  constexpr int npc = 1;
  constexpr int npv[1]{ 3 };
  mpi_sub_process(argc, argv, npc, npv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  i32 world_rank = 0;
  i32 world_size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  constexpr auto n = 1024;

  ASSERT_TRY(auto tmp_buffer, Buffer::create(4 * page_ceil(n * sizeof(vertex_t))));
  auto* tmp_memory = tmp_buffer.data();

  ASSERT_TRY(auto scc_and_graph, fuzzy_global_scc_id_and_graph(1, n, tmp_memory, 1));
  auto [buffer, graph, scc] = std::move(scc_and_graph);

//  test_all_gather_graph(graph, TrivialSlicePart{ n });
//  test_all_gather_graph(graph, BalancedSlicePart{ n });
//  test_all_gather_graph(graph, CyclicPart{ n });
//  test_all_gather_graph(graph, BlockCyclicPart{ n });

//  test_all_gather_graph_part(graph, TrivialSlicePart{ n });
//  test_all_gather_graph_part(graph, BalancedSlicePart{ n });
//  test_all_gather_graph_part(graph, CyclicPart{ n });
//  test_all_gather_graph_part(graph, BlockCyclicPart{ n });
}
