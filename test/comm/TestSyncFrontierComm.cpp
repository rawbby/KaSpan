#include <comm/SyncFrontierComm.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/Arithmetic.hpp>
#include <util/ScopeGuard.hpp>

#include <kamping/communicator.hpp>
#include <mpi.h>
#include <sys/wait.h>

#include <cstdio>
#include <random>
#include <vector>

namespace {

/// sanitize the state of the frontier communicator pre and post communication
template<class Part>
auto
sanitized_communication(kamping::Communicator<> const& comm, Part const& part, SyncFrontierComm<Part>& frontier) -> Result<bool>
{
  auto const rank = comm.rank();
  auto const size = comm.size();

  auto const& recv_buf    = frontier.recv_buf();    // vector
  auto const  recv_counts = frontier.recv_counts(); // buffer
  auto const  recv_displs = frontier.recv_displs(); // buffer
  auto const& send_buf    = frontier.send_buf();    // vector
  auto const  send_counts = frontier.send_counts(); // buffer
  auto const  send_displs = frontier.send_displs(); // buffer

  // pre checks
  ASSERT_EQ(send_displs[0], 0);
  size_t total_send_count = 0;
  for (auto const send_count : send_counts)
    total_send_count += send_count;
  ASSERT_EQ(total_send_count, send_buf.size());

  for (auto v : recv_buf)
    ASSERT_EQ(part.world_rank_of(v), rank);

  for (size_t i = 0; i < size; ++i) {
    size_t total_messages_to_i = 0;
    for (size_t j = 0; j < send_buf.size(); ++j)
      if (part.world_rank_of(send_buf[j]) == i)
        ++total_messages_to_i;
    ASSERT_EQ(total_messages_to_i, send_counts[i], "i=%zu", i);
  }

  auto const recv_buf_size_before = recv_buf.size();

  auto const result = frontier.template communicate<false>(comm, part);

  if (result) { // only check if the buffer where changed

    // Prefix-sum structure
    ASSERT_GE(send_counts[0], 0);
    ASSERT_GE(recv_counts[0], 0);
    ASSERT_EQ(send_displs[0], 0);
    ASSERT_EQ(recv_displs[0], 0);
    for (size_t i = 1; i < size; ++i) {
      ASSERT_GE(send_counts[i], 0);
      ASSERT_GE(recv_counts[i], 0);
      ASSERT_EQ(send_displs[i], send_displs[i - 1] + send_counts[i - 1]);
      ASSERT_EQ(recv_displs[i], recv_displs[i - 1] + recv_counts[i - 1]);
    }

    // Ends match totals
    auto const send_end = send_displs[size - 1] + send_counts[size - 1];
    ASSERT_EQ(send_end, send_buf.size());

    size_t total_recv_count = 0;
    for (auto const recv_count : recv_counts)
      total_recv_count += recv_count;

    auto const recv_end = recv_displs[size - 1] + recv_counts[size - 1];
    ASSERT_EQ(recv_end, total_recv_count);
    ASSERT_EQ(recv_buf.size(), recv_buf_size_before + total_recv_count);

    // Received entries belong to self
    for (auto v : recv_buf)
      ASSERT_EQ(part.world_rank_of(v), rank);

    for (size_t i = 0; i < size; ++i) {
      size_t total_messages_to_i = 0;
      for (size_t j = 0; j < send_buf.size(); ++j)
        if (part.world_rank_of(send_buf[j]) == i)
          ++total_messages_to_i;
      ASSERT_EQ(total_messages_to_i, send_counts[i], "i=%zu", i);
    }
  }

  frontier.clear();
  return result;
}

/// generate random but seeded messages so every rank has the same collection
auto
random_messages(u64 seed, size_t max_count = 4096, u64 value_range = 8192) -> std::vector<u64>
{
  auto rng      = std::mt19937_64{ seed };
  auto cnt_dist = std::uniform_int_distribution<size_t>(0, max_count);
  auto val_dist = std::uniform_int_distribution<u64>(0, value_range - 1);

  auto const count = cnt_dist(rng);

  std::vector<u64> out;
  out.reserve(count);

  for (size_t i = 0; i < count; ++i)
    out.push_back(val_dist(rng));

  return out;
}

} // namespace

/// test different configurations
/// notice: Fuzzy = 0 means hard coded sample
template<bool Relaxed, u64 Fuzzy, int Runs, class Part>
void
test_kernel(kamping::Communicator<> const& comm, Part const& part, char const* part_name)
{
  auto frontier_result = SyncFrontierComm<Part>::create(comm);
  ASSERT(frontier_result.has_value());
  auto frontier = std::move(frontier_result.value());

  if (comm.rank() == 0) {
    std::cout << "[Test] " << Runs << " Runs";
    std::cout << ' ' << part_name;
    if (Relaxed)
      std::cout << " Relaxed";
    if (Fuzzy)
      std::cout << " Fuzzy (seed: " << Fuzzy << ')';
    else
      std::cout << " HardCoded";
    std::cout << '\n';
  }

  for (int run = 0; run < Runs; ++run) {
    auto const msgs = [run]() -> std::array<std::vector<u64>, 4> {
      if (Fuzzy) {
        return {
          random_messages(0xea5d2a82 + run ^ Fuzzy),
          random_messages(0x88fb7320 + run ^ Fuzzy),
          random_messages(0x15599dbf + run ^ Fuzzy),
          random_messages(0x654a4731 + run ^ Fuzzy)
        };
      }
      return {
        std::vector<u64>{ 12, 4, 15, 0, 8, 10, 13 },
        std::vector<u64>{ 9, 5, 4, 11, 6, 11, 10, 3, 15, 6, 15, 2, 2 },
        std::vector<u64>{},
        std::vector<u64>{ 0, 0, 6, 15 }
      };
    }();

    std::vector<u64> got;

    size_t expected_relaxed_count = 0;
    for (auto const v : msgs[comm.rank()]) {
      if (Relaxed) {
        if (part.world_rank_of(v) == comm.rank())
          ++expected_relaxed_count;
        frontier.push_relaxed(comm, part, v);
      } else {
        frontier.push(part, v);
      }
    }

    while (frontier.has_next()) {
      auto const v = frontier.next();
      ASSERT_EQ(part.world_rank_of(v), comm.rank());
      got.push_back(v);
    }

    ASSERT_LE(got.size(), msgs[comm.rank()].size());

    auto const relaxed_count = got.size();
    ASSERT_EQ(expected_relaxed_count, relaxed_count);

    auto const result0 = sanitized_communication(comm, part, frontier);
    ASSERT(result0.has_value());
    // ASSERT(result0.value());

    while (frontier.has_next()) {
      auto const v = frontier.next();
      got.push_back(v);
    }

    size_t expected_message_count = 0;
    for (size_t r = 0; r < comm.size(); ++r)
      for (auto const v : msgs[r])
        if (part.world_rank_of(v) == comm.rank())
          ++expected_message_count;

    auto const message_count = got.size();
    ASSERT_EQ(expected_message_count, message_count);
  }

  ASSERT(not frontier.communicate(comm, part));
}

auto
main(int argc, char** argv) -> int
{
  constexpr auto n = std::numeric_limits<size_t>::max();

  constexpr int npc      = 4;
  constexpr int npv[npc] = { 1, 2, 3, 4 };
  mpi_sub_process(argc, argv, npc, npv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto const comm     = kamping::Communicator{};
  auto const part     = CyclicPart{ n, comm.rank(), comm.size() };
  auto const blk_part = BlockCyclicPart{ n, comm.rank(), comm.size() };
  auto const bs_part  = BalancedSlicePart{ n, comm.rank(), comm.size() };
  auto const ts_part  = TrivialSlicePart{ n, comm.rank(), comm.size() };

  constexpr int  NO_FUZZ         = 0;
  constexpr int  HARD_CODED_RUNS = 3;
  constexpr int  FUZZ_RUNS       = 128;
  constexpr bool RELAXED         = true;
  constexpr bool NON_RELAXED     = false;
  constexpr int  FUZZ_SEEDS[8]{ 0x8067e9, 0xdf00dd, 0xd0ecf0, 0x22d80b, 0x17e615, 0xd59eef, 0x215869, 0xa2a5a5 };

  test_kernel<NON_RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, bs_part, "BalancedSlicePart");
  test_kernel<NON_RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, ts_part, "TrivialSlicePart");
  test_kernel<NON_RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, part, "CyclicPart");
  test_kernel<NON_RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, blk_part, "BlockCyclicPart");

  test_kernel<RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, bs_part, "BalancedSlicePart");
  test_kernel<RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, ts_part, "TrivialSlicePart");
  test_kernel<RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, part, "CyclicPart");
  test_kernel<RELAXED, NO_FUZZ, HARD_CODED_RUNS>(comm, blk_part, "BlockCyclicPart");

  test_kernel<NON_RELAXED, FUZZ_SEEDS[2], FUZZ_RUNS>(comm, bs_part, "BalancedSlicePart");
  test_kernel<NON_RELAXED, FUZZ_SEEDS[3], FUZZ_RUNS>(comm, ts_part, "TrivialSlicePart");
  test_kernel<NON_RELAXED, FUZZ_SEEDS[0], FUZZ_RUNS>(comm, part, "CyclicPart");
  test_kernel<NON_RELAXED, FUZZ_SEEDS[1], FUZZ_RUNS>(comm, blk_part, "BlockCyclicPart");

  test_kernel<RELAXED, FUZZ_SEEDS[6], FUZZ_RUNS>(comm, bs_part, "BalancedSlicePart");
  test_kernel<RELAXED, FUZZ_SEEDS[7], FUZZ_RUNS>(comm, ts_part, "TrivialSlicePart");
  test_kernel<RELAXED, FUZZ_SEEDS[4], FUZZ_RUNS>(comm, part, "CyclicPart");
  test_kernel<RELAXED, FUZZ_SEEDS[5], FUZZ_RUNS>(comm, blk_part, "BlockCyclicPart");
}
