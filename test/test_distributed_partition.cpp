#include <distributed/Partition.hpp>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace distributed;

constexpr std::size_t exhaustive_n_max  = 1000;
constexpr std::size_t first_sample      = 200;
constexpr std::size_t last_sample       = 200;
constexpr std::size_t stride_factor     = 600;
constexpr int         randomized_trials = 256;

template<typename... Args>
[[noreturn]] void
fail(const std::string& msg, Args&&... args)
{
  std::cerr << msg;
  ((std::cerr << " " << std::forward<Args>(args)), ...);
  std::cerr << std::endl;
  std::exit(1);
}

std::vector<std::size_t>
sampled_indices(std::size_t n)
{
  std::vector<std::size_t> indices;
  if (n <= exhaustive_n_max) {
    indices.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
      indices.push_back(i);
  } else {
    const std::size_t stride = std::max<std::size_t>(1, n / stride_factor);
    indices.reserve(first_sample + n / stride + last_sample + 8);
    for (std::size_t i = 0; i < std::min(first_sample, n); ++i)
      indices.push_back(i);
    for (std::size_t i = first_sample; i + last_sample < n; i += stride)
      indices.push_back(i);
    for (std::size_t i = n - std::min(last_sample, n); i < n; ++i)
      indices.push_back(i);
  }
  return indices;
}

template<class Partition, class Fn>
void
for_all_ranks(std::size_t n, std::size_t world_size, Fn&& fn)
{
  for (std::size_t rank = 0; rank < world_size; ++rank) {
    fn(Partition(n, rank, world_size), rank);
  }
}
template<class Partition, class Fn>
void
for_all_ranks_blocksize(std::size_t n, std::size_t world_size, std::size_t block_size, Fn&& fn)
{
  for (std::size_t rank = 0; rank < world_size; ++rank) {
    fn(Partition(n, rank, world_size, block_size), rank);
  }
}

static_assert(PartitionConcept<CyclicPartition>);
static_assert(PartitionConcept<TrivialSlicePartition>);
static_assert(PartitionConcept<BalancedSlicePartition>);
static_assert(PartitionConcept<BlockCyclicPartition>);

template<class Partition>
void
test_basic_invariants(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  if (part.size() == 0 != part.empty())
    fail("size() == 0 <=> empty() violated", pname, "n=", n, "ws=", world_size, "rank=", rank);
  if (part.size() > n)
    fail("size() > n violated", pname, "n=", n, "ws=", world_size, "rank=", rank, "size=", part.size());
  if constexpr (Partition::continuous) {
    if (!(part.begin <= part.end && part.end <= n && part.end - part.begin == part.size()))
      fail("continuous begin/end invariant violated", pname, "n=", n, "ws=", world_size, "rank=", rank);
  }
}

template<class Partition>
void
test_contains_vs_world_rank_of(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  for (std::size_t i : sampled_indices(n)) {
    const bool  c  = part.contains(i);
    std::size_t wr = part.world_rank_of(i);
    if (c && wr != rank)
      fail("contains(i) true but world_rank_of(i) != rank", pname, "n=", n, "ws=", world_size, "rank=", rank, "i=", i, "wr=", wr);
    if (!c && wr == rank)
      fail("contains(i) false but world_rank_of(i) == rank", pname, "n=", n, "ws=", world_size, "rank=", rank, "i=", i);
  }
}

template<class Partition>
void
test_rank_select_roundtrip(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  // For all i with contains(i), select(rank(i)) == i
  for (std::size_t i : sampled_indices(n)) {
    if (part.contains(i)) {
      std::size_t k = part.rank(i);
      std::size_t g = part.select(k);
      if (g != i)
        fail("select(rank(i)) != i", pname, "n=", n, "ws=", world_size, "rank=", rank, "i=", i, "k=", k, "g=", g);
    }
  }
  // For all k in [0, size()), contains(select(k)) and rank(select(k)) == k
  for (std::size_t k : sampled_indices(part.size())) {
    if (k >= part.size())
      continue;
    std::size_t i = part.select(k);
    if (!part.contains(i))
      fail("contains(select(k)) false", pname, "n=", n, "ws=", world_size, "rank=", rank, "k=", k, "i=", i);
    if (part.rank(i) != k)
      fail("rank(select(k)) != k", pname, "n=", n, "ws=", world_size, "rank=", rank, "k=", k, "i=", i, "rank(i)=", part.rank(i));
  }
}

template<class Partition>
void
test_select_monotonic(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  if (part.size() <= 1)
    return;
  for (std::size_t k = 0; k + 1 < part.size(); ++k) {
    std::size_t i0 = part.select(k), i1 = part.select(k + 1);
    if (i0 >= i1)
      fail("select(k) not strictly increasing", pname, "n=", n, "ws=", world_size, "rank=", rank, "k=", k, "i0=", i0, "i1=", i1);
  }
}

template<class Partition>
void
test_local_ordering_consecutive(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  for (std::size_t i : sampled_indices(n)) {
    if (part.contains(i) && i + 1 < n && part.contains(i + 1)) {
      std::size_t r0 = part.rank(i), r1 = part.rank(i + 1);
      if (r1 != r0 + 1)
        fail("consecutive owned elements, but rank(i+1) != rank(i)+1", pname, "n=", n, "ws=", world_size, "rank=", rank, "i=", i, "r0=", r0, "r1=", r1);
    }
  }
}

template<class Partition>
void
test_size_equals_count_contains(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  if (n > exhaustive_n_max)
    return;
  std::size_t count = 0;
  for (std::size_t i = 0; i < n; ++i)
    count += part.contains(i) ? 1 : 0;
  if (count != part.size())
    fail("size() != number of contains(i)", pname, "n=", n, "ws=", world_size, "rank=", rank, "size=", part.size(), "count=", count);
}

template<class Partition>
void
test_global_uniqueness(std::size_t n, std::size_t world_size, const std::string& pname)
{
  for (std::size_t i : sampled_indices(n)) {
    std::size_t c = 0;
    for (std::size_t rank = 0; rank < world_size; ++rank) {
      Partition part(n, rank, world_size);
      c += part.contains(i) ? 1 : 0;
    }
    if (c != 1)
      fail("index contained in != 1 partition", pname, "n=", n, "ws=", world_size, "i=", i, "contained=", c);
  }
}

template<class Partition>
void
test_size_sum_is_n(std::size_t n, std::size_t world_size, const std::string& pname)
{
  std::size_t total = 0;
  for (std::size_t rank = 0; rank < world_size; ++rank) {
    Partition part(n, rank, world_size);
    total += part.size();
  }
  if (total != n)
    fail("sum of size() over ranks != n", pname, "n=", n, "ws=", world_size, "sum=", total);
}

template<class Partition>
void
test_continuous_invariants(const Partition& part, std::size_t n, std::size_t world_size, std::size_t rank, const std::string& pname)
{
  if constexpr (Partition::continuous) {
    for (std::size_t i : sampled_indices(n)) {
      bool in = part.begin <= i && i < part.end;
      if (part.contains(i) != in)
        fail("continuous: contains(i) != (begin <= i < end)", pname, "n=", n, "ws=", world_size, "rank=", rank, "i=", i, "begin=", part.begin, "end=", part.end);
      if (part.world_rank_of(i) == rank != in)
        fail("continuous: world_rank_of(i) == rank != (begin <= i < end)", pname, "n=", n, "ws=", world_size, "rank=", rank, "i=", i, "wr=", part.world_rank_of(i));
    }
  }
}

void
test_cross_partition_equal(std::size_t n, std::size_t world_size)
{
  // block_cyclic with block_size=1 == cyclic
  for (std::size_t rank = 0; rank < world_size; ++rank) {
    BlockCyclicPartition bc(n, rank, world_size, 1);
    CyclicPartition       cp(n, rank, world_size);
    for (std::size_t i : sampled_indices(n)) {
      if (bc.contains(i) != cp.contains(i))
        fail("block_cyclic(block=1) != cyclic: contains", "n=", n, "ws=", world_size, "rank=", rank, "i=", i);
      if (bc.contains(i)) {
        if (bc.rank(i) != cp.rank(i))
          fail("block_cyclic(block=1) != cyclic: rank", "n=", n, "ws=", world_size, "rank=", rank, "i=", i);
      }
    }
  }
  // trivial_slice == balanced_slice when n % world_size == 0
  if (n % world_size == 0) {
    for (std::size_t rank = 0; rank < world_size; ++rank) {
      TrivialSlicePartition  t(n, rank, world_size);
      BalancedSlicePartition b(n, rank, world_size);
      for (std::size_t i : sampled_indices(n)) {
        if (t.contains(i) != b.contains(i))
          fail("trivial_slice != balanced_slice: contains", "n=", n, "ws=", world_size, "rank=", rank, "i=", i);
      }
    }
  }
}

void
test_block_cyclic_size(std::size_t n, std::size_t world_size, std::size_t block_size)
{
  for (std::size_t rank = 0; rank < world_size; ++rank) {
    BlockCyclicPartition part(n, rank, world_size, block_size);

    // Hand-calculate expected size
    const std::size_t num_blocks = (n + block_size - 1) / block_size;
    const std::size_t my_blocks  = num_blocks > rank ? (num_blocks - rank + world_size - 1) / world_size : 0;
    std::size_t       expected   = my_blocks * block_size;
    const std::size_t last_block = num_blocks - 1;
    if (my_blocks > 0 && last_block % world_size == rank) {
      const std::size_t start = last_block * block_size;
      if (n < start + block_size)
        expected -= start + block_size - n;
    }
    if (expected > n)
      expected = n;
    if (part.size() != expected)
      fail("block_cyclic: size() formula wrong", "n=", n, "ws=", world_size, "rank=", rank, "block=", block_size, "size=", part.size(), "expected=", expected);

    // Check last index is n-1 if this rank owns the last element
    if (part.size() > 0 && part.select(part.size() - 1) == n - 1) {
      if (!part.contains(n - 1))
        fail("block_cyclic: last select points to n-1, but contains(n-1) is false", "n=", n, "ws=", world_size, "rank=", rank, "block=", block_size);
    }
  }
}

template<class Partition>
void
test_partition_suite(std::size_t n, std::size_t world_size, const std::string& pname)
{
  for (std::size_t rank = 0; rank < world_size; ++rank) {
    Partition part(n, rank, world_size);
    test_basic_invariants(part, n, world_size, rank, pname);
    test_contains_vs_world_rank_of(part, n, world_size, rank, pname);
    test_rank_select_roundtrip(part, n, world_size, rank, pname);
    test_select_monotonic(part, n, world_size, rank, pname);
    test_local_ordering_consecutive(part, n, world_size, rank, pname);
    test_size_equals_count_contains(part, n, world_size, rank, pname);
    test_continuous_invariants(part, n, world_size, rank, pname);
  }
  test_global_uniqueness<Partition>(n, world_size, pname);
  test_size_sum_is_n<Partition>(n, world_size, pname);
  test_cross_partition_equal(n, world_size);
}

// with block size variants
template<class Partition>
void
test_block_cyclic_suite(std::size_t n, std::size_t world_size, const std::string& pname)
{
  constexpr std::size_t block_sizes[] = { 1, 2, 3, 7, 64, 512, 1024 };
  for (std::size_t block : block_sizes) {
    for (std::size_t rank = 0; rank < world_size; ++rank) {
      Partition part(n, rank, world_size, block);
      test_basic_invariants(part, n, world_size, rank, pname);
      test_contains_vs_world_rank_of(part, n, world_size, rank, pname);
      test_rank_select_roundtrip(part, n, world_size, rank, pname);
      test_select_monotonic(part, n, world_size, rank, pname);
      test_local_ordering_consecutive(part, n, world_size, rank, pname);
      test_size_equals_count_contains(part, n, world_size, rank, pname);
      test_block_cyclic_size(n, world_size, block);
    }
    test_global_uniqueness<Partition>(n, world_size, pname);
    test_size_sum_is_n<Partition>(n, world_size, pname);
    test_cross_partition_equal(n, world_size);
  }
}

int
main()
{
  const auto   seed = static_cast<std::size_t>(std::random_device{}());
  std::mt19937 rng{ seed };

  std::cout << "seed used: 0x" << std::hex << seed << std::endl;

  for (std::size_t n = 1; n < 64; ++n)
    for (std::size_t ws = 1; ws < 16; ++ws) {
      test_partition_suite<CyclicPartition>(n, ws, "cyclic_partition");
      test_block_cyclic_suite<BlockCyclicPartition>(n, ws, "block_cyclic_partition");
      test_partition_suite<TrivialSlicePartition>(n, ws, "trivial_slice_partition");
      test_partition_suite<BalancedSlicePartition>(n, ws, "balanced_slice_partition");
    }

  for (int trial = 0; trial < randomized_trials; ++trial) {
    const std::size_t n  = 64 + rng() % 4000;
    const std::size_t ws = 1 + rng() % 64;
    test_partition_suite<CyclicPartition>(n, ws, "cyclic_partition");
    test_block_cyclic_suite<BlockCyclicPartition>(n, ws, "block_cyclic_partition");
    test_partition_suite<TrivialSlicePartition>(n, ws, "trivial_slice_partition");
    test_partition_suite<BalancedSlicePartition>(n, ws, "balanced_slice_partition");
  }

  std::cout << "All partition tests passed." << std::endl;
  return 0;
}
