#include <algorithm>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/queue_builder.hpp>
#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <numeric>
#include <print>
#include <scc/base.hpp>
#include <vector>

void
await_messages(auto& mq, auto&& on_message)
{
  constexpr auto timeout_8s = static_cast<i64>(8'000'000'000ll);

  auto const now_ns = [] {
    using clock = std::chrono::steady_clock;
    using unit  = std::chrono::nanoseconds;
    return std::chrono::duration_cast<unit>(clock::now().time_since_epoch()).count();
  };

  auto const deadline    = now_ns() + timeout_8s;
  auto const on_progress = [=] {
    ASSERT_LT(now_ns(), deadline);
  };

  do {
    on_progress();
    mq.poll_throttled(on_message);
  } while (not mq.terminate(on_message, on_progress));
}

void
test_single_all_to_all()
{
  auto received = std::vector<i32>{};
  received.reserve(mpi_world_size - 1);

  auto on_message = [&](auto env) {
    for (auto val : env.message) {
      received.push_back(val);
    }
  };

  auto mq = briefkasten::BufferedMessageQueueBuilder<i32>{}.build();
  for (i32 i = 0; i < mpi_world_size; ++i) {
    if (i != mpi_world_rank) {
      mq.post_message_blocking(mpi_world_rank, i, on_message);
    }
  }

  await_messages(mq, on_message);

  ASSERT_EQ(received.size(), mpi_world_size - 1);

  auto expected = std::vector<i32>{};
  expected.reserve(mpi_world_size - 1);
  for (i32 i = 0; i < mpi_world_size; ++i) {
    if (i != mpi_world_rank) {
      expected.push_back(i);
    }
  }

  std::ranges::sort(received);
  for (auto [a, b] : std::views::zip(received, expected)) {
    ASSERT_EQ(a, b);
  }
}

void
test_reactivation()
{
  constexpr auto num_messages = 10;

  i32 const send_target = (mpi_world_rank + 1) % mpi_world_size;
  i32 const recv_source = (mpi_world_rank - 1 + mpi_world_size) % mpi_world_size;

  auto received = std::vector<i32>{};
  received.reserve(std::max(mpi_world_size, num_messages));
  auto expected = std::vector<i32>{};
  expected.reserve(num_messages);

  auto on_message = [&](auto env) {
    for (auto val : env.message) {
      received.push_back(val);
    }
  };

  for (i32 i = 0; i < num_messages; ++i) {
    expected.push_back(recv_source * num_messages + i);
  }

  auto mq = briefkasten::BufferedMessageQueueBuilder<i32>{}.build();

  // send a bunch of messages that hopefully dont leak and reset
  {
    for (i32 i = 0; i < mpi_world_size; ++i) {
      if (i != mpi_world_rank) {
        mq.post_message_blocking(mpi_world_size * num_messages + i, i, on_message);
      }
    }
    await_messages(mq, on_message);
    ASSERT_EQ(received.size(), static_cast<size_t>(mpi_world_size - 1));
    mpi_basic_barrier();
    mq.reactivate();
    received.clear();
  }

  for (i32 i = 0; i < num_messages; ++i) {
    mq.post_message_blocking(mpi_world_rank * num_messages + i, send_target, on_message);
  }

  await_messages(mq, on_message);

  ASSERT_EQ(received.size(), num_messages);
  std::ranges::sort(received);
  for (auto [a, b] : std::views::zip(received, expected)) {
    ASSERT_EQ(a, b);
  }
}

void
test_neighbour_multiple()
{
  constexpr auto num_messages = 10;

  i32 const send_target = (mpi_world_rank + 1) % mpi_world_size;
  i32 const recv_source = (mpi_world_rank - 1 + mpi_world_size) % mpi_world_size;

  auto received = std::vector<i32>{};
  received.reserve(num_messages);
  auto expected = std::vector<i32>{};
  expected.reserve(num_messages);

  auto on_message = [&](auto env) {
    for (auto val : env.message) {
      received.push_back(val);
    }
  };

  for (i32 i = 0; i < num_messages; ++i) {
    expected.push_back(recv_source * num_messages + i);
  }

  auto mq = briefkasten::BufferedMessageQueueBuilder<i32>{}.build();
  for (i32 i = 0; i < num_messages; ++i) {
    mq.post_message_blocking(mpi_world_rank * num_messages + i, send_target, on_message);
  }

  await_messages(mq, on_message);

  std::ranges::sort(received);
  ASSERT_EQ(received.size(), static_cast<size_t>(num_messages));
  for (auto [a, b] : std::views::zip(received, expected)) {
    ASSERT_EQ(a, b);
  }
}

void
test_no_communication()
{
  auto received   = std::vector<i32>{};
  auto on_message = [&](auto env) {
    for (auto val : env.message) {
      received.push_back(val);
    }
  };

  auto mq = briefkasten::BufferedMessageQueueBuilder<i32>{}.build();
  await_messages(mq, on_message);

  ASSERT_EQ(received.size(), 0u);
}

void
test_single_self_message()
{
  auto received = std::vector<i32>{};
  received.reserve(1);

  auto on_message = [&](auto env) {
    for (auto val : env.message) {
      ASSERT_EQ(val, mpi_world_rank);
      received.push_back(val);
    }
  };

  auto mq = briefkasten::BufferedMessageQueueBuilder<i32>{}.build();
  mq.post_message_blocking(mpi_world_rank, mpi_world_rank, on_message);
  await_messages(mq, on_message);

  ASSERT_EQ(received.size(), 1u);
}

int
main(int argc, char** argv)
{
  constexpr i32 npc = 8;
  constexpr i32 npv[npc]{ 1, 2, 3, 4, 6, 8, 12, 16 };
  mpi_sub_process(argc, argv, npc, npv);
  KASPAN_DEFAULT_INIT();

  for (i32 i = 0; i < 32; ++i) {
    test_single_all_to_all();
    mpi_basic_barrier();
    test_reactivation();
    mpi_basic_barrier();
    test_neighbour_multiple();
    mpi_basic_barrier();
    test_no_communication();
    mpi_basic_barrier();
    test_single_self_message();
    mpi_basic_barrier();
  }
}
