#include "kaspan/mpi_basic/type.hpp"
#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/assert_false.hpp>
#include <kaspan/debug/assert_lt.hpp>
#include <kaspan/debug/assert_ne.hpp>
#include <kaspan/debug/assert_true.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/edge_frontier.hpp>
#include <kaspan/scc/part.hpp>
#include <kaspan/util/arithmetic.hpp>

using namespace kaspan;

void
test_trivial_hard_coded()
{
  auto const part = trivial_slice_part{ mpi_basic::world_size };
  ASSERT_EQ(part.local_n(), 1);

  auto front = edge_frontier::create();

  ASSERT_EQ(front.recv_buffer.size(), 0);
  ASSERT_EQ(front.send_buffer.size(), 0);

  ASSERT_NOT(front.comm(part));

  ASSERT_EQ(front.recv_buffer.size(), 0);
  ASSERT_EQ(front.send_buffer.size(), 0);

  for (vertex_t u = 0; u < part.n; ++u) {
    auto const r = part.world_rank_of(u);
    ASSERT_EQ(r, u);
    front.push(r, { u, 0 });
  }

  for (i32 r = 0; r < mpi_basic::world_size; ++r) {
    ASSERT_EQ(front.send_counts[r], 1);
  }

  ASSERT_EQ(front.recv_buffer.size(), 0);
  ASSERT_EQ(front.send_buffer.size(), mpi_basic::world_size);

  ASSERT(front.comm(part));
  for (i32 r = 0; r < mpi_basic::world_size; ++r) {
    ASSERT_EQ(front.send_counts[r], 0);
    ASSERT_EQ(front.recv_counts[r], 1);
    ASSERT_EQ(front.send_displs[r], r);
    ASSERT_EQ(front.recv_displs[r], r);
  }

  ASSERT_EQ(front.recv_buffer.size(), mpi_basic::world_size);
  ASSERT_EQ(front.send_buffer.size(), 0);

  ASSERT_NOT(front.comm(part));

  ASSERT_EQ(front.recv_buffer.size(), mpi_basic::world_size);
  ASSERT_EQ(front.send_buffer.size(), 0);

  i32 message_count = 0;
  while (front.has_next()) {
    auto const [u, v] = front.next();
    auto const r      = part.world_rank_of(u);
    ASSERT_EQ(r, u);
    ASSERT_EQ(r, mpi_basic::world_rank);
    ASSERT_EQ(v, 0);
    ++message_count;
  }

  ASSERT_EQ(message_count, mpi_basic::world_size);
}

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  int rank = -1;
  int size = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  ASSERT_NE(rank, -1);
  ASSERT_NE(size, -1);
  ASSERT_EQ(rank, mpi_basic::world_rank);
  ASSERT_EQ(size, mpi_basic::world_size);
  ASSERT_LT(rank, size);

  test_trivial_hard_coded();
}
