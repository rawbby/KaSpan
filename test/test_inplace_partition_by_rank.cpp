#include <debug/sub_process.hpp>
#include <memory/stack_accessor.hpp>
#include <scc/base.hpp>
#include <util/mpi_basic.hpp>

#include <algorithm>
#include <random>

struct Item
{
  i32 dest;
  i32 id;
};

static void
check_case(Item* send_buffer, MPI_Count send_count, MPI_Count const* send_counts, MPI_Aint* send_displs)
{
  ASSERT_IN_RANGE(mpi_world_rank, 0, mpi_world_size);

  send_displs[0] = 0;
  for (i32 r = 1; r < mpi_world_size; ++r) {
    send_displs[r] = send_displs[r - 1] + send_counts[r - 1];
  }

  mpi_basic_inplace_partition_by_rank<Item>(send_buffer, send_counts, send_displs, [](Item const& x) {
    return x.dest;
  });

  auto  buffer = Buffer(2 * page_ceil<MPI_Count>(mpi_world_size));
  auto* memory = buffer.data();

  auto* beg = borrow<MPI_Count>(memory, mpi_world_size);
  auto* end = borrow<MPI_Count>(memory, mpi_world_size);

  for (i32 r = 0; r < mpi_world_size; ++r) {
    beg[r] = static_cast<MPI_Count>(send_displs[r]);
    end[r] = beg[r] + send_counts[r];
  }

  ASSERT(beg[0] == 0);
  for (i32 r = 1; r < mpi_world_size; ++r) {
    ASSERT_EQ(end[r - 1], beg[r]);
  }

  if (send_count > 0) {
    ASSERT_EQ(end[mpi_world_size - 1], send_count);
  }

  for (i32 r = 0; r < mpi_world_size; ++r) {
    for (size_t i = beg[r]; i < end[r]; ++i) {
      ASSERT(send_buffer[i].dest == r);
    }
  }
}

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  // random
  {
    auto rng  = std::mt19937(123);
    auto dist = std::uniform_int_distribution(0, mpi_world_size - 1);

    constexpr i32 min_send_count = 10;
    i32 const     max_send_count = 2 * (mpi_world_size - 1) + min_send_count;

    auto [cb, send_counts, send_displs] = mpi_basic_counts_and_displs();
    auto [sb, send_stack]               = StackAccessor<Item>::create(max_send_count);

    for (i32 trial = 0; trial < 1024; ++trial) {
      std::memset(send_counts, 0, mpi_world_size * sizeof(MPI_Count));
      send_stack.clear();

      i32 send_count = dist(rng) + dist(rng) + min_send_count;

      ASSERT_IN_RANGE(send_count, 0, max_send_count + 1);

      for (i32 i = 0; i < send_count; ++i) {
        i32 const destination = dist(rng);
        send_stack.push({ destination, i });
        ++send_counts[destination];
      }

      std::shuffle(send_stack.data(), send_stack.data() + send_count, rng);
      check_case(send_stack.data(), send_count, send_counts, send_displs);
    }
  }

  // all to one rank
  {
    auto const target_rank = std::min(2, mpi_world_size - 1);

    constexpr auto send_count = 7;

    auto [cb, send_counts, send_displs] = mpi_basic_counts_and_displs();
    auto [sb, send_stack]               = StackAccessor<Item>::create(send_count);

    std::memset(send_counts, 0, mpi_world_size * sizeof(MPI_Count));
    send_counts[target_rank] = send_count;

    for (i32 i = 0; i < send_count; ++i) {
      send_stack.push({ target_rank, i });
    }

    check_case(send_stack.data(), send_count, send_counts, send_displs);
  }

  // zeros everywhere
  {
    auto [cb, send_counts, send_displs] = mpi_basic_counts_and_displs();
    auto [sb, send_stack]               = StackAccessor<Item>::create(0);

    std::memset(send_counts, 0, mpi_world_size * sizeof(MPI_Count));
    check_case(send_stack.data(), 0, send_counts, send_displs);
  }
}
