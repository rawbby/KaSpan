#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/mpi_basic/allgatherv.hpp>
#include <kaspan/mpi_basic/alltoallv.hpp>
#include <kaspan/mpi_basic/counts_and_displs.hpp>
#include <kaspan/mpi_basic/displs.hpp>
#include <kaspan/mpi_basic/mpi_basic.hpp>
#include <kaspan/scc/base.hpp>

using namespace kaspan;

void
test_allgatherv_nullptr()
{
  // Test case: rank 0 has no data to send (nullptr send_buffer), but other ranks might
  auto const rank = mpi_basic::world_rank;
  auto const size = mpi_basic::world_size;

  // Each rank contributes rank * 2 elements
  MPI_Count send_count = rank * 2;

  auto send_buffer = make_buffer<vertex_t>(send_count);
  auto send_ptr    = static_cast<vertex_t*>(send_buffer.data());

  // Fill send buffer with rank ID
  for (MPI_Count i = 0; i < send_count; ++i) {
    send_ptr[i] = rank;
  }

  // Gather counts from all ranks
  auto [b1, recv_counts, recv_displs] = mpi_basic::counts_and_displs();
  mpi_basic::allgatherv_counts(send_count, recv_counts);
  auto recv_count = mpi_basic::displs(recv_counts, recv_displs);

  auto recv_buffer = make_buffer<vertex_t>(recv_count);
  auto recv_ptr    = static_cast<vertex_t*>(recv_buffer.data());

  // This should work even if send_ptr is nullptr (when send_count == 0)
  mpi_basic::allgatherv(send_ptr, send_count, recv_ptr, recv_counts, recv_displs);

  // Verify received data
  for (int r = 0; r < size; ++r) {
    MPI_Count expected_count = r * 2;
    MPI_Count offset         = recv_displs[r];
    for (MPI_Count i = 0; i < expected_count; ++i) {
      ASSERT_EQ(recv_ptr[offset + i], r, "rank {} pos {}", r, i);
    }
  }
}

void
test_allgatherv_all_zero()
{
  // Test case: all ranks have zero elements
  auto const rank = mpi_basic::world_rank;

  MPI_Count send_count = 0;
  vertex_t* send_ptr   = nullptr; // nullptr is valid when count is 0

  auto [b1, recv_counts, recv_displs] = mpi_basic::counts_and_displs();
  mpi_basic::allgatherv_counts(send_count, recv_counts);
  auto recv_count = mpi_basic::displs(recv_counts, recv_displs);

  ASSERT_EQ(recv_count, 0, "Expected total recv_count to be 0");

  vertex_t* recv_ptr = nullptr; // nullptr when total count is 0

  // This should not crash even with nullptr buffers
  mpi_basic::allgatherv(send_ptr, send_count, recv_ptr, recv_counts, recv_displs);
}

void
test_alltoallv_nullptr()
{
  // Test case: some ranks have no data to send/receive
  auto const rank = mpi_basic::world_rank;
  auto const size = mpi_basic::world_size;

  auto [sb, send_counts, send_displs] = mpi_basic::counts_and_displs();
  auto [rb, recv_counts, recv_displs] = mpi_basic::counts_and_displs();

  // Only rank 0 sends one element to each other rank
  std::memset(send_counts, 0, size * sizeof(MPI_Count));
  if (rank == 0) {
    for (int r = 1; r < size; ++r) {
      send_counts[r] = 1;
    }
  }

  auto send_count = mpi_basic::displs(send_counts, send_displs);

  // Prepare send buffer
  auto send_buffer = make_buffer<vertex_t>(send_count);
  auto send_ptr    = static_cast<vertex_t*>(send_buffer.data());
  for (MPI_Count i = 0; i < send_count; ++i) {
    send_ptr[i] = rank * 100;
  }

  // Exchange counts
  mpi_basic::alltoallv_counts(send_counts, recv_counts);
  auto recv_count = mpi_basic::displs(recv_counts, recv_displs);

  auto recv_buffer = make_buffer<vertex_t>(recv_count);
  auto recv_ptr    = static_cast<vertex_t*>(recv_buffer.data());

  // This should work even with nullptr (when counts are 0 for some ranks)
  mpi_basic::alltoallv(send_ptr, send_counts, send_displs, recv_ptr, recv_counts, recv_displs);

  // Verify: non-zero ranks should receive one element from rank 0
  if (rank > 0) {
    ASSERT_EQ(recv_count, 1, "Rank {} should receive 1 element", rank);
    ASSERT_EQ(recv_ptr[0], 0, "Rank {} should receive 0 from rank 0", rank);
  } else {
    ASSERT_EQ(recv_count, 0, "Rank 0 should receive nothing");
  }
}

void
test_alltoallv_all_zero()
{
  // Test case: no communication at all
  auto const rank = mpi_basic::world_rank;
  auto const size = mpi_basic::world_size;

  auto [sb, send_counts, send_displs] = mpi_basic::counts_and_displs();
  auto [rb, recv_counts, recv_displs] = mpi_basic::counts_and_displs();

  std::memset(send_counts, 0, size * sizeof(MPI_Count));

  auto send_count = mpi_basic::displs(send_counts, send_displs);
  ASSERT_EQ(send_count, 0, "Expected send_count to be 0");

  vertex_t* send_ptr = nullptr;

  mpi_basic::alltoallv_counts(send_counts, recv_counts);
  auto recv_count = mpi_basic::displs(recv_counts, recv_displs);
  ASSERT_EQ(recv_count, 0, "Expected recv_count to be 0");

  vertex_t* recv_ptr = nullptr;

  // This should not crash with all-nullptr and all-zero counts
  mpi_basic::alltoallv(send_ptr, send_counts, send_displs, recv_ptr, recv_counts, recv_displs);
}

int
main(int argc, char** argv)
{
  // Test with 1, 3, and 8 processes
  constexpr int npc      = 3;
  constexpr int npv[npc] = { 1, 3, 8 };
  mpi_sub_process(argc, argv, npc, npv);

  KASPAN_DEFAULT_INIT();

  test_allgatherv_nullptr();
  test_allgatherv_all_zero();
  test_alltoallv_nullptr();
  test_alltoallv_all_zero();

  return 0;
}
