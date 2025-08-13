#pragma once

#include <concepts>
#include <cstddef>

namespace distributed {

/**
 * PartitionConcept
 *
 * Models a subset S of {0, ..., n-1}.
 *
 * Requirements:
 * - Public member: n (size_t) - total element count
 * - bool contains(i): true iff i ∈ S
 * - size_t rank(i): local index of i within S (requires contains(i) == true)
 * - size_t select(k): global index for local k in [0, size())
 * - size_t size(): |S| (number of elements in S)
 * - static constexpr bool continuous: true if S is a contiguous interval
 * - If continuous: members begin, end (interval [begin, end))
 */
template<class T>
concept PartitionConcept =
  requires(T t, std::size_t i) {
    { t.n } -> std::convertible_to<std::size_t>;
    { t.contains(i) } -> std::convertible_to<bool>;
    { t.rank(i) } -> std::convertible_to<std::size_t>;
    { t.select(i) } -> std::convertible_to<std::size_t>;
    { t.size() } -> std::convertible_to<std::size_t>;
    { T::continuous } -> std::convertible_to<bool>;
  } && (!T::continuous || requires(T t) {
    { t.begin } -> std::convertible_to<std::size_t>;
    { t.end } -> std::convertible_to<std::size_t>;
  });

/**
 * WorldPartitionConcept
 *
 * Extends PartitionConcept to represent a complete, disjoint partitioning of {0, ..., n-1}
 * among world_size parts. Each index is owned by exactly one world_rank.
 *
 * Additional Requirements:
 * - Constructor: T(n, world_rank, world_size)
 * - size_t world_rank_of(i): owning rank for global index i
 */
template<class T>
concept WorldPartitionConcept =
  PartitionConcept<T> &&
  requires(T t, std::size_t i) {
    { T{ i, i, i } } -> std::convertible_to<T>;
    { t.world_rank_of(i) } -> std::convertible_to<std::size_t>;
  };

/**
 * ExplicitContinuousPartition
 *
 * Represents a contiguous interval [begin, end) subset of {0, ..., n-1}.
 */
struct ExplicitContinuousPartition
{
  static constexpr bool continuous = true;

  std::size_t n;
  std::size_t begin;
  std::size_t end;

  explicit constexpr ExplicitContinuousPartition(std::size_t n) noexcept
    : n(n)
    , begin(0)
    , end(0)
  {
  }

  constexpr ExplicitContinuousPartition(std::size_t n, std::size_t begin, std::size_t end) noexcept
    : n(n)
    , begin(begin)
    , end(end)
  {
  }

  [[nodiscard]] constexpr bool contains(std::size_t i) const noexcept
  {
    return i >= begin && i < end;
  }

  [[nodiscard]] constexpr std::size_t rank(std::size_t i) const noexcept
  {
    return i - begin;
  }

  [[nodiscard]] constexpr std::size_t select(std::size_t k) const noexcept
  {
    return begin + k;
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    return end - begin;
  }
};

/**
 * CyclicPartition
 *
 * Indices owned by rank r: { i | i % world_size == r }
 */
struct CyclicPartition
{
  static constexpr bool continuous = false;

  std::size_t n;
  std::size_t world_rank;
  std::size_t world_size;

  constexpr CyclicPartition(std::size_t n, std::size_t world_rank, std::size_t world_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
  }

  [[nodiscard]] constexpr bool contains(std::size_t i) const noexcept
  {
    return world_size == 1 || (i % world_size) == world_rank;
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    return world_size == 1 ? 0 : (i % world_size);
  }

  [[nodiscard]] constexpr std::size_t rank(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return i;
    return (i - world_rank) / world_size;
  }

  [[nodiscard]] constexpr std::size_t select(std::size_t k) const noexcept
  {
    if (world_size == 1)
      return k;
    return world_rank + k * world_size;
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    if (world_size == 1)
      return n;
    if (n <= world_rank)
      return 0;
    return (n - 1 - world_rank) / world_size + 1;
  }
};

/**
 * BlockCyclicPartition
 *
 * Blocks of consecutive indices of size block_size are distributed cyclically among ranks.
 */
struct BlockCyclicPartition
{
  static constexpr bool continuous = false;

  std::size_t n;
  std::size_t world_rank;
  std::size_t world_size;
  std::size_t block_size;

  static constexpr std::size_t default_block_size = 512;

  constexpr BlockCyclicPartition(std::size_t n,
                                 std::size_t world_rank,
                                 std::size_t world_size,
                                 std::size_t block_size = default_block_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
    , block_size(block_size)
  {
  }

  [[nodiscard]] constexpr bool contains(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return true;
    const std::size_t block = i / block_size;
    return (block % world_size) == world_rank;
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return 0;
    const std::size_t block = i / block_size;
    return block % world_size;
  }

  // Precondition: contains(i)
  [[nodiscard]] constexpr std::size_t rank(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return i;
    const std::size_t block                        = i / block_size;
    const std::size_t offset_in_block              = i % block_size;
    const std::size_t num_full_owned_blocks_before = block / world_size;
    return num_full_owned_blocks_before * block_size + offset_in_block;
  }

  // Precondition: k < size()
  [[nodiscard]] constexpr std::size_t select(std::size_t k) const noexcept
  {
    if (world_size == 1)
      return k;
    const std::size_t local_block        = k / block_size; // which owned block (0-based)
    const std::size_t offset             = k % block_size; // offset within that block
    const std::size_t global_block       = local_block * world_size + world_rank;
    const std::size_t global_block_start = global_block * block_size;
    return global_block_start + offset;
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    if (world_size == 1)
      return n;
    if (n == 0)
      return 0;

    const std::size_t num_blocks = (n + block_size - 1) / block_size; // ceil(n / block_size)
    if (num_blocks == 0)
      return 0;

    // Number of owned blocks
    std::size_t owned_blocks = 0;
    if (num_blocks > world_rank)
      owned_blocks = (num_blocks - 1 - world_rank) / world_size + 1;

    if (owned_blocks == 0)
      return 0;

    // Elements = (owned_blocks - 1) full blocks + size of last owned block
    const std::size_t last_owned_block       = world_rank + (owned_blocks - 1) * world_size;
    const bool        owns_global_last_block = last_owned_block == num_blocks - 1;
    const std::size_t last_block_size        = owns_global_last_block ? n - last_owned_block * block_size : block_size;

    return (owned_blocks - 1) * block_size + last_block_size;
  }
};

/**
 * TrivialSlicePartition
 *
 * Contiguous slicing into world_size ranges:
 * - All ranks 0..world_size-2 get exactly base = n / world_size elements.
 * - The last rank gets base + (n % world_size) elements.
 * This is a simple contiguous partition by index as referenced in the README.
 */
struct TrivialSlicePartition : ExplicitContinuousPartition
{
  std::size_t world_rank;
  std::size_t world_size;

  constexpr TrivialSlicePartition(std::size_t n, std::size_t world_rank, std::size_t world_size) noexcept
    : ExplicitContinuousPartition(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
    if (world_size == 1) {
      begin = 0;
      end   = n;
      return;
    }

    const std::size_t base  = n / world_size;
    const std::size_t start = world_rank * base;
    begin                   = start;
    if (world_rank + 1 == world_size) {
      end = n;
    } else {
      end = start + base;
    }
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return 0;

    const std::size_t base = n / world_size;
    if (base == 0)
      return world_size - 1;

    const std::size_t cutoff = base * (world_size - 1);
    if (i < cutoff)
      return i / base;
    return world_size - 1;
  }
};

/**
 * BalancedSlicePartition
 *
 * Contiguous slicing with size difference at most one:
 * - First (n % world_size) ranks get (base + 1) elements.
 * - Remaining ranks get base elements, where base = n / world_size.
 */
struct BalancedSlicePartition : ExplicitContinuousPartition
{
  std::size_t world_rank;
  std::size_t world_size;

  constexpr BalancedSlicePartition(std::size_t n, std::size_t world_rank, std::size_t world_size) noexcept
    : ExplicitContinuousPartition(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
    if (world_size == 1) {
      begin = 0;
      end   = n;
      return;
    }

    const std::size_t base = n / world_size;
    const std::size_t rem  = n % world_size;

    if (world_rank < rem) {
      begin = world_rank * (base + 1);
      end   = begin + (base + 1);
    } else {
      begin = rem * (base + 1) + (world_rank - rem) * base;
      end   = begin + base;
    }
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return 0;

    const std::size_t base = n / world_size;
    const std::size_t rem  = n % world_size;

    if (rem == 0) {
      return base == 0 ? 0 : i / base;
    }

    const std::size_t split = (base + 1) * rem;
    if (i < split)
      return i / (base + 1);

    return rem + (i - split) / base;
  }
};

} // namespace distributed
