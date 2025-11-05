#include <../../core/essential/include/debug/Assert.hpp>
#include <../../core/essential/include/debug/SubProcess.hpp>
#include <comm/SyncAlltoall.hpp>
#include <scc/part.hpp>
#include <memory/buffer.hpp>
#include <util/scope_guard.hpp>

i32
main(i32 argc, char** argv)
{
  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  i32 world_rank = 0;
  i32 world_size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  ASSERT_TRY(auto buffer, Buffer::create(4 * page_ceil(world_size * sizeof(i32))));
  void* memory = buffer.data();

  auto const part = TrivialSlicePart{ static_cast<vertex_t>(world_size), world_rank, world_size };

  auto edge_comm = SyncAlltoallvBase{ SyncEdgeComm{ part }, memory };

  for (vertex_t i = 0; i < part.n; ++i)
    edge_comm.push({ i, 0 });

  ASSERT(edge_comm.communicate());
  size_t message_count = 0;
  while (edge_comm.has_next()) {
    auto const [u, v] = edge_comm.next();
    ASSERT_EQ(part.world_rank_of(u), world_rank);
    ASSERT_EQ(v, 0);
    ++message_count;
  }
  ASSERT_EQ(message_count, world_size);
}
