#include <scc/SCC.hpp>

#include <graph/DistributedGraph.hpp>
#include <util/ScopeGuard.hpp>

#include <buffer/Buffer.hpp>

#include <kamping/collectives/allgather.hpp>
#include <kamping/communicator.hpp>

#include <mpi.h>

VoidResult
run(int argc, char** argv)
{
  ASSERT_TRY(argc == 3, IO_ERROR, "Usage: ./scc_cpu <manifest> <run_times>");

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());
  kamping::Communicator comm{};

  RESULT_TRY(auto const manifest, Manifest::load(argv[1]));
  TrivialSlicePartition const partition{ manifest.graph_node_count, comm.rank(), comm.size() };
  RESULT_TRY(auto const graph, DistributedGraph<>::load(comm, partition, manifest));

  auto const run_times = atoi(argv[2]);

  for (int i = 0; i < run_times; ++i)
    RESULT_TRY(scc_detection(comm, graph));

  return VoidResult::success();
}

int
main(int argc, char** argv)
{
  return static_cast<int>(run(argc, argv).error_or_ok());
}
