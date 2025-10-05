#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <ispan/scc_detection.hpp>

#include <kamping/communicator.hpp>
#include <mpi.h>

auto
load_graph(kamping::Communicator<>& comm)
{
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, "rgg2d;n=16;m=32"));
  ASSERT_TRY(auto graph, AllGatherGraph(comm, graph_part));
  return graph;
}

int
main(int args, char** argv)
{
  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  // auto const graph = load_graph(comm);

  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, "rgg2d;n=1000;r=0.2"));
  ASSERT_TRY(const auto graph, AllGatherGraph(comm, graph_part));

  scc_detection(graph, 100, comm.rank(), comm.size());
}
