#pragma once

#include <buffer/Buffer.hpp>
#include <util/Arithmetic.hpp>

#include <concepts>
#include <cstddef>
#include <mpi.h>

template<class T>
concept PartitionConcept =

  (std::is_move_constructible_v<T> and std::is_move_assignable_v<T>)

  and

  requires(T t, size_t i) {
    { t.n } -> std::convertible_to<size_t>;
    { t.contains(i) } -> std::convertible_to<bool>;
    { t.rank(i) } -> std::convertible_to<size_t>;
    { t.select(i) } -> std::convertible_to<size_t>;
    { t.size() } -> std::convertible_to<size_t>;
    { T::continuous } -> std::convertible_to<bool>;
  }

  and

  (!T::continuous or requires(T t) {
    { t.begin } -> std::convertible_to<size_t>;
    { t.end } -> std::convertible_to<size_t>;
  });

template<class T>
concept WorldPartitionConcept =

  PartitionConcept<T>

  and

  requires(T t, size_t i) {
    { t.world_size } -> std::convertible_to<size_t>;
    { t.world_rank } -> std::convertible_to<size_t>;
    { t.world_rank_of(i) } -> std::convertible_to<size_t>;
  };

struct ExplicitContinuousPartition
{
  static constexpr bool continuous = true;

  size_t n     = 0;
  size_t begin = 0;
  size_t end   = 0;

  constexpr ExplicitContinuousPartition() noexcept  = default;
  constexpr ~ExplicitContinuousPartition() noexcept = default;

  explicit constexpr ExplicitContinuousPartition(size_t n) noexcept
    : n(n)
  {
  }

  constexpr ExplicitContinuousPartition(size_t n, size_t begin, size_t end) noexcept
    : n(n)
    , begin(begin)
    , end(end)
  {
  }

  constexpr ExplicitContinuousPartition(ExplicitContinuousPartition const&) noexcept = default;
  constexpr ExplicitContinuousPartition(ExplicitContinuousPartition&&) noexcept      = default;

  constexpr auto operator=(ExplicitContinuousPartition const&) noexcept -> ExplicitContinuousPartition& = default;
  constexpr auto operator=(ExplicitContinuousPartition&&) noexcept -> ExplicitContinuousPartition&      = default;

  [[nodiscard]] constexpr auto contains(size_t i) const noexcept -> bool
  {
    return i >= begin && i < end;
  }

  [[nodiscard]] constexpr auto rank(size_t i) const noexcept -> size_t
  {
    return i - begin;
  }

  [[nodiscard]] constexpr auto select(size_t k) const noexcept -> size_t
  {
    return begin + k;
  }

  [[nodiscard]] constexpr auto size() const noexcept -> size_t
  {
    return end - begin;
  }
};

struct CyclicPartition final
{
  static constexpr bool continuous = false;

  size_t n          = 0;
  size_t world_rank = 0;
  size_t world_size = 0;

  constexpr CyclicPartition() noexcept  = default;
  constexpr ~CyclicPartition() noexcept = default;

  constexpr CyclicPartition(size_t n, size_t world_rank, size_t world_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
  }

  constexpr CyclicPartition(CyclicPartition const&) noexcept = default;
  constexpr CyclicPartition(CyclicPartition&&) noexcept      = default;

  constexpr auto operator=(CyclicPartition const&) noexcept -> CyclicPartition& = default;
  constexpr auto operator=(CyclicPartition&&) noexcept -> CyclicPartition&      = default;

  [[nodiscard]] constexpr auto contains(size_t i) const noexcept -> bool
  {
    return world_size == 1 || (i % world_size) == world_rank;
  }

  [[nodiscard]] constexpr auto world_rank_of(size_t i) const noexcept -> size_t
  {
    return world_size == 1 ? 0 : (i % world_size);
  }

  [[nodiscard]] constexpr auto rank(size_t i) const noexcept -> size_t
  {
    if (world_size == 1)
      return i;
    return (i - world_rank) / world_size;
  }

  [[nodiscard]] constexpr auto select(size_t k) const noexcept -> size_t
  {
    if (world_size == 1)
      return k;
    return world_rank + (k * world_size);
  }

  [[nodiscard]] constexpr auto size() const noexcept -> size_t
  {
    if (world_size == 1)
      return n;
    if (n <= world_rank)
      return 0;
    return ((n - 1 - world_rank) / world_size) + 1;
  }
};

struct BlockCyclicPartition final
{
  static constexpr bool   continuous         = false;
  static constexpr size_t default_block_size = 512;

  size_t n          = 0;
  size_t world_rank = 0;
  size_t world_size = 0;
  size_t block_size = 0;

  constexpr BlockCyclicPartition() noexcept  = default;
  constexpr ~BlockCyclicPartition() noexcept = default;

  constexpr BlockCyclicPartition(size_t n, size_t world_rank, size_t world_size, size_t block_size = default_block_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
    , block_size(block_size)
  {
  }

  constexpr BlockCyclicPartition(BlockCyclicPartition const&) noexcept = default;
  constexpr BlockCyclicPartition(BlockCyclicPartition&&) noexcept      = default;

  constexpr auto operator=(BlockCyclicPartition const&) noexcept -> BlockCyclicPartition& = default;
  constexpr auto operator=(BlockCyclicPartition&&) noexcept -> BlockCyclicPartition&      = default;

  [[nodiscard]] constexpr auto contains(size_t i) const noexcept -> bool
  {
    if (world_size == 1)
      return true;
    size_t const block = i / block_size;
    return (block % world_size) == world_rank;
  }

  [[nodiscard]] constexpr auto world_rank_of(size_t i) const noexcept -> size_t
  {
    if (world_size == 1)
      return 0;
    size_t const block = i / block_size;
    return block % world_size;
  }

  // Precondition: contains(i)
  [[nodiscard]] constexpr auto rank(size_t i) const noexcept -> size_t
  {
    if (world_size == 1)
      return i;
    size_t const block                        = i / block_size;
    size_t const offset_in_block              = i % block_size;
    size_t const num_full_owned_blocks_before = block / world_size;
    return num_full_owned_blocks_before * block_size + offset_in_block;
  }

  // Precondition: k < size()
  [[nodiscard]] constexpr auto select(size_t k) const noexcept -> size_t
  {
    if (world_size == 1)
      return k;
    size_t const local_block        = k / block_size; // which owned block (0-based)
    size_t const offset             = k % block_size; // offset within that block
    size_t const global_block       = local_block * world_size + world_rank;
    size_t const global_block_start = global_block * block_size;
    return global_block_start + offset;
  }

  [[nodiscard]] constexpr auto size() const noexcept -> size_t
  {
    if (world_size == 1)
      return n;
    if (n == 0)
      return 0;

    size_t const num_blocks = (n + block_size - 1) / block_size; // ceil(n / block_size)
    if (num_blocks == 0)
      return 0;

    // Number of owned blocks
    size_t owned_blocks = 0;
    if (num_blocks > world_rank)
      owned_blocks = (num_blocks - 1 - world_rank) / world_size + 1;

    if (owned_blocks == 0)
      return 0;

    // Elements = (owned_blocks - 1) full blocks + size of last owned block
    size_t const last_owned_block       = world_rank + ((owned_blocks - 1) * world_size);
    bool const   owns_global_last_block = last_owned_block == num_blocks - 1;
    size_t const last_block_size        = owns_global_last_block ? n - (last_owned_block * block_size) : block_size;

    return ((owned_blocks - 1) * block_size) + last_block_size;
  }
};

struct TrivialSlicePartition final : ExplicitContinuousPartition
{
  size_t world_rank = 0;
  size_t world_size = 0;

  constexpr TrivialSlicePartition() noexcept  = default;
  constexpr ~TrivialSlicePartition() noexcept = default;

  constexpr TrivialSlicePartition(size_t n, size_t world_rank, size_t world_size) noexcept
    : ExplicitContinuousPartition(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
    if (world_size == 1) {
      begin = 0;
      end   = n;
      return;
    }

    size_t const base  = n / world_size;
    size_t const start = world_rank * base;
    begin              = start;
    if (world_rank + 1 == world_size) {
      end = n;
    } else {
      end = start + base;
    }
  }

  constexpr TrivialSlicePartition(TrivialSlicePartition const&) noexcept = default;
  constexpr TrivialSlicePartition(TrivialSlicePartition&&) noexcept      = default;

  constexpr auto operator=(TrivialSlicePartition const&) noexcept -> TrivialSlicePartition& = default;
  constexpr auto operator=(TrivialSlicePartition&&) noexcept -> TrivialSlicePartition&      = default;

  [[nodiscard]] constexpr auto world_rank_of(size_t i) const noexcept -> size_t
  {
    if (world_size == 1)
      return 0;

    size_t const base = n / world_size;
    if (base == 0)
      return world_size - 1;

    size_t const cutoff = base * (world_size - 1);
    if (i < cutoff)
      return i / base;
    return world_size - 1;
  }
};

struct BalancedSlicePartition final : ExplicitContinuousPartition
{
  size_t world_rank;
  size_t world_size;

  constexpr BalancedSlicePartition() noexcept  = default;
  constexpr ~BalancedSlicePartition() noexcept = default;

  constexpr BalancedSlicePartition(size_t n, size_t world_rank, size_t world_size) noexcept
    : ExplicitContinuousPartition(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
    if (world_size == 1) {
      begin = 0;
      end   = n;
      return;
    }

    size_t const base = n / world_size;
    size_t const rem  = n % world_size;

    if (world_rank < rem) {
      begin = world_rank * (base + 1);
      end   = begin + (base + 1);
    } else {
      begin = rem * (base + 1) + (world_rank - rem) * base;
      end   = begin + base;
    }
  }

  constexpr BalancedSlicePartition(BalancedSlicePartition const&) noexcept = default;
  constexpr BalancedSlicePartition(BalancedSlicePartition&&) noexcept      = default;

  constexpr auto operator=(BalancedSlicePartition const&) noexcept -> BalancedSlicePartition& = default;
  constexpr auto operator=(BalancedSlicePartition&&) noexcept -> BalancedSlicePartition&      = default;

  [[nodiscard]] constexpr auto world_rank_of(size_t i) const noexcept -> size_t
  {
    if (world_size == 1)
      return 0;

    size_t const base = n / world_size;
    size_t const rem  = n % world_size;

    if (rem == 0) {
      return base == 0 ? 0 : i / base;
    }

    size_t const split = (base + 1) * rem;
    if (i < split)
      return i / (base + 1);

    return rem + ((i - split) / base);
  }
};

struct ExplicitContinuousWorldPartition final : ExplicitContinuousPartition
{
  size_t    world_rank = 0;
  size_t    world_size = 0;
  U64Buffer partition;

  ExplicitContinuousWorldPartition()  = default;
  ~ExplicitContinuousWorldPartition() = default;

  ExplicitContinuousWorldPartition(ExplicitContinuousWorldPartition const&) = delete;
  ExplicitContinuousWorldPartition(ExplicitContinuousWorldPartition&&)      = default;

  auto operator=(ExplicitContinuousWorldPartition const&) -> ExplicitContinuousWorldPartition& = delete;
  auto operator=(ExplicitContinuousWorldPartition&&) -> ExplicitContinuousWorldPartition&      = default;

  static auto create(size_t n, size_t begin, size_t end, size_t world_rank, size_t world_size) -> Result<ExplicitContinuousWorldPartition>
  {
    ExplicitContinuousWorldPartition result{};
    RESULT_TRY(result.partition, U64Buffer::zeroes(2 * world_size));
    result.n          = n;
    result.begin      = begin;
    result.end        = end;
    result.world_rank = world_rank;
    result.world_size = world_size;

    u64 const local_range[2]{ begin, end };
    MPI_Allgather(&local_range, 2, MPI_UINT64_T, result.partition.data(), 2, MPI_UINT64_T, MPI_COMM_WORLD);
    return result;
  }

  [[nodiscard]] auto world_rank_of(size_t i) const -> size_t
  {
    for (size_t r = 0; r < world_size; ++r)
      if (i >= partition[r * 2] and i < partition[r * 2 + 1])
        return r;
    return -1;
  }
};

struct ExplicitSortedContinuousWorldPartition final : ExplicitContinuousPartition
{
  size_t    world_rank = 0;
  size_t    world_size = 0;
  U64Buffer partition;

  ExplicitSortedContinuousWorldPartition()  = default;
  ~ExplicitSortedContinuousWorldPartition() = default;

  ExplicitSortedContinuousWorldPartition(ExplicitSortedContinuousWorldPartition const&) = delete;
  ExplicitSortedContinuousWorldPartition(ExplicitSortedContinuousWorldPartition&&)      = default;

  auto operator=(ExplicitSortedContinuousWorldPartition const&) -> ExplicitSortedContinuousWorldPartition& = delete;
  auto operator=(ExplicitSortedContinuousWorldPartition&&) -> ExplicitSortedContinuousWorldPartition&      = default;

  static auto create(size_t n, size_t begin, size_t end, size_t world_rank, size_t world_size) -> Result<ExplicitSortedContinuousWorldPartition>
  {
    ExplicitSortedContinuousWorldPartition result{};
    RESULT_TRY(result.partition, U64Buffer::zeroes(world_size));
    result.n          = n;
    result.begin      = begin;
    result.end        = end;
    result.world_rank = world_rank;
    result.world_size = world_size;

    u64 const local_end = end;
    MPI_Allgather(&local_end, 1, MPI_UINT64_T, result.partition.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);
    return result;
  }

  [[nodiscard]] auto world_rank_of(size_t i) const -> size_t
  {
    auto r = i * static_cast<u64>(world_size) / static_cast<u64>(n);
    while (r > 0 and i < partition[r - 1])
      --r;
    while (r + 1 < world_size and i >= partition[r])
      ++r;
    return r;
  }
};
