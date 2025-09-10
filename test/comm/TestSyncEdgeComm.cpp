#include <comm/SyncEdgeComm.hpp>
#include <graph/Partition.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>

#include <kamping/communicator.hpp>

int
main(int argc, char** argv)
{
  namespace kmp = kamping;

  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto       comm = kmp::Communicator{};
  auto const part = TrivialSlicePartition{ comm.size(), comm.rank(), comm.size() };

  auto impl   = SyncEdgeComm{ part };
  auto result = SyncAlltoallvBase<decltype(impl)>::create(comm, impl);
  ASSERT(result.has_value());

  auto edge_comm = std::move(result.value());

  for (size_t i = 0; i < comm.size(); ++i)
    edge_comm.push({ i, 0 });

  ASSERT(edge_comm.communicate(comm));
  size_t message_count = 0;
  while (edge_comm.has_next()) {
    auto const [v, u] = edge_comm.next();
    ASSERT_EQ(part.world_rank_of(v), comm.rank());
    ASSERT_EQ(u, 0);
    ++message_count;
  }
  ASSERT_EQ(message_count, comm.size());
}
