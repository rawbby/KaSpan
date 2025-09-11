#include <buffer/Buffer.hpp>
#include <graph/DistributedGraph.hpp>
#include <scc/SCC.hpp>
#include <util/ScopeGuard.hpp>

#include <kamping/collectives/allgather.hpp>
#include <kamping/communicator.hpp>

#include <mpi.h>

auto
main(int argc, char** argv) -> int
{
  ASSERT(argc == 2, "Usage: ./scc_cpu <manifest>");

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());
  kamping::Communicator comm{};

  ASSERT_TRY(auto const manifest, Manifest::load(argv[1]));
  TrivialSlicePartition const partition{ manifest.graph_node_count, comm.rank(), comm.size() };
  ASSERT_TRY(auto const graph, DistributedGraph<>::load(partition, manifest));

  ASSERT_TRY(auto scc_id, U64Buffer::create(manifest.graph_node_count));
  ASSERT_TRY(scc_detection(comm, graph, scc_id));
}
