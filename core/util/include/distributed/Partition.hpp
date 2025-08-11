#pragma once

#include <concepts>
#include <cstddef>

/**
 * @brief PartitionConcept
 *
 * Models a subset S of the index set {0, ..., n-1}, providing efficient access by rank and select.
 *
 * Requirements:
 * - Public member: n (size_t) - total element count
 * - bool contains(i): true iff i element S
 * - size_t rank(i): local index of i within S (requires contains(i) == true)
 * - size_t select(k): global index for local k element [0, size())
 * - size_t size(): |S| (number of elements in S)
 * - static constexpr bool continuous: true if S is a contiguous interval
 *   - If continuous: members begin, end (interval [begin, end))
 *
 * No requirement for S to cover all or be disjoint with other subsets.
 */
template<class T>
concept PartitionConcept =
  requires(T t, std::size_t i) {
    { t.n } -> std::convertible_to<std::size_t>;
    { t.rank(i) } -> std::convertible_to<std::size_t>;
    { t.select(i) } -> std::convertible_to<std::size_t>;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.contains(i) } -> std::convertible_to<bool>;
    { T::continuous } -> std::convertible_to<bool>;
  } &&
  (not T::continuous || requires(T t) {
    { t.begin } -> std::convertible_to<std::size_t>;
    { t.end } -> std::convertible_to<std::size_t>;
  });

/**
 * @brief WorldPartitionConcept
 *
 * Extends PartitionConcept for world-aware partitions:
 * - Represents a division of {0, ..., n-1} among multiple non-overlapping partitions (per world-rank).
 * - Supports world-rank construction and lookup.
 *
 * Additional Requirements:
 * - Constructor: T(n, world_rank, world_size)
 * - size_t world_rank_of(i): returns owning rank for index i
 *
 * Guarantees disjoint, complete partitioning across world-size partitions.
 */
template<class T>
concept WorldPartitionConcept =
  PartitionConcept<T> &&
  requires(T t, std::size_t i) {
    { T{ i, i, i } } -> std::convertible_to<T>;
    { t.world_rank_of(i) } -> std::convertible_to<std::size_t>;
  };

/**
 * @brief ExplicitContinuousPartition
 *
 * Concrete implementation of PartitionConcept for a contiguous interval [begin, end) subset of {0, ..., n-1}.
 *
 * Specialization:
 * - continuous == true
 * - Models a single contiguous subset using [begin, end)
 *
 * See PartitionConcept for general contract.
 */
struct ExplicitContinuousPartition
{
  static constexpr bool continuous = true;

  std::size_t n;
  std::size_t begin;
  std::size_t end;

  explicit ExplicitContinuousPartition(std::size_t n)
    : n(n)
    , begin()
    , end()
  {
  }

  ExplicitContinuousPartition(std::size_t n, std::size_t begin, std::size_t end)
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
    if (i <= begin)
      return 0;
    if (i >= end)
      return end - begin;
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
 * @brief CyclicPartition
 *
 * Concrete implementation of WorldPartitionConcept for cyclic partitioning of {0, ..., n-1} across multiple world ranks.
 *
 * Each partition owns indices i where i % world_size == world_rank.
 *
 * Properties:
 * - continuous == false
 * - n: total element count
 * - world_rank: index of this partition (0 <= world_rank < world_size)
 * - world_size: total number of partitions
 *
 * See WorldPartitionConcept for general contract.
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
    return world_size == 1 || i % world_size == world_rank;
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    return world_size == 1 ? 0 : i % world_size;
  }

  [[nodiscard]] constexpr std::size_t rank(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return i;
    if (i == 0)
      return 0;
    if (i <= world_rank)
      return 0;
    return (i - 1 - world_rank) / world_size + 1;
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
 * @brief BlockCyclicPartition
 *
 * Concrete implementation of WorldPartitionConcept for block-cyclic partitioning of {0, ..., n-1} across multiple world ranks.
 *
 * Each partition owns blocks of consecutive indices of size block_size, distributed in cyclic order among world ranks.
 * For block b (0-based), block is assigned to rank (b % world_size).
 *
 * Properties:
 * - continuous == false
 * - n: total element count
 * - world_rank: index of this partition (0 <= world_rank < world_size)
 * - world_size: total number of partitions
 * - block_size: number of consecutive elements per block
 *
 * See WorldPartitionConcept for general contract.
 */
struct BlockCyclicPartition
{
  static constexpr bool continuous = false;

  std::size_t n;
  std::size_t world_rank;
  std::size_t world_size;
  std::size_t block_size;

  static constexpr std::size_t default_block_size = 512;

  constexpr BlockCyclicPartition(std::size_t n, std::size_t world_rank, std::size_t world_size, std::size_t block_size = default_block_size) noexcept
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
    return block % world_size == world_rank;
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return 0;
    const std::size_t block = i / block_size;
    return block % world_size;
  }

  [[nodiscard]] constexpr std::size_t rank(std::size_t i) const noexcept
  {
    if (!contains(i))
      return size();

    const std::size_t block                  = i / block_size;
    const std::size_t offset_in_block        = i % block_size;
    const std::size_t num_full_blocks_before = block / world_size;
    return num_full_blocks_before * block_size + offset_in_block;
  }

  [[nodiscard]] constexpr std::size_t select(std::size_t k) const noexcept
  {
    if (world_size == 1)
      return k;

    const std::size_t block        = k / block_size;
    const std::size_t offset       = k % block_size;
    const std::size_t global_block = block * world_size + world_rank;
    const std::size_t i            = global_block * block_size + offset;
    return i < n ? i : n;
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    if (world_size == 1)
      return n;

    std::size_t       count          = 0;
    const std::size_t num_blocks     = (n + block_size - 1) / block_size;
    const std::size_t my_full_blocks = (num_blocks + world_size - world_rank - 1) / world_size;
    count += my_full_blocks * block_size;

    const std::size_t last_block = num_blocks - 1;
    if (last_block % world_size == world_rank) {
      const std::size_t start = last_block * block_size;
      if (n < start + block_size)
        count -= start + block_size - n;
    }
    if (count > n)
      count = n;
    return count;
  }
};

/**
 * @brief TrivialSlicePartition
 *
 * Concrete implementation of WorldPartitionConcept that divides {0, ..., n-1} into up to (world_size - 1) contiguous slices of near equal size.
 *
 * If n divides evenly by world_size, all slices are of equal size. Otherwise, the last slice may be smaller.
 *
 * Properties:
 * - continuous == true
 * - n: total element count
 * - world_rank: index of this partition (0 <= world_rank < world_size)
 * - world_size: total number of partitions
 *
 * See WorldPartitionConcept for general contract.
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

    if (n % world_size == 0) {
      const std::size_t count = n / world_size;
      begin                   = world_rank * count;
      end                     = begin + count;
      return;
    }

    const std::size_t blocks = world_size - 1;
    const std::size_t base   = blocks ? n / blocks : n;

    // last world_rank
    if (world_rank + 1 == world_size) {
      begin = base * blocks;
      end   = n;
      return;
    }

    begin = world_rank * base;
    end   = begin + base;
  }

  [[nodiscard]] constexpr std::size_t world_rank_of(std::size_t i) const noexcept
  {
    if (world_size == 1)
      return 0u;

    if (n % world_size == 0)
      return i / (n / world_size);

    const std::size_t blocks = world_size - 1;
    const std::size_t base   = blocks ? n / blocks : n;
    if (i < base * blocks)
      return i / base;

    return world_size - 1;
  }
};

/**
 * @brief BalancedSlicePartition
 *
 * Concrete implementation of WorldPartitionConcept that divides {0, ..., n-1} into world_size contiguous slices with size difference at most one.
 *
 * The first (n % world_size) slices have one more element than the remaining slices, ensuring the most even distribution possible.
 *
 * Properties:
 * - continuous == true
 * - n: total element count
 * - world_rank: index of this partition (0 <= world_rank < world_size)
 * - world_size: total number of partitions
 *
 * See WorldPartitionConcept for general contract.
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

    if (rem == 0)
      return i / base;

    const std::size_t split = (base + 1) * rem;

    if (i < split)
      return i / (base + 1);

    return rem + (i - split) / base;
  }
};
