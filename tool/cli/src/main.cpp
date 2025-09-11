#include <buffer/Buffer.hpp>
#include <graph/DistributedGraph.hpp>
#include <scc/SCC.hpp>
#include <util/ScopeGuard.hpp>

#include <kamping/collectives/allgather.hpp>
#include <kamping/communicator.hpp>

#include <mpi.h>

auto
run(int argc, char** argv) -> VoidResult
{
  RESULT_ASSERT(argc == 2, IO_ERROR, "Usage: ./scc_cpu <manifest>");

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());
  kamping::Communicator comm{};

  RESULT_TRY(auto const manifest, Manifest::load(argv[1]));
  TrivialSlicePartition const partition{ manifest.graph_node_count, comm.rank(), comm.size() };
  RESULT_TRY(auto const graph, DistributedGraph<>::load(partition, manifest));

  RESULT_TRY(auto scc_id, U64Buffer::create(manifest.graph_node_count));
  RESULT_TRY(scc_detection(comm, graph, scc_id));

  return VoidResult::success();
}

int
main(int argc, char** argv)
{
  return static_cast<int>(run(argc, argv).error_or_ok());
}
