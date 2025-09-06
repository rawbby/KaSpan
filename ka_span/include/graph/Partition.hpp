#pragma once

#include "util/Arithmetic.hpp"
#include "util/MpiTuple.hpp"
#include "util/ScopeGuard.hpp"

#include <concepts>
#include <cstddef>

#include <mpi.h>

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
  requires(T t, size_t i) {
    { t.n } -> std::convertible_to<size_t>;
    { t.contains(i) } -> std::convertible_to<bool>;
    { t.rank(i) } -> std::convertible_to<size_t>;
    { t.select(i) } -> std::convertible_to<size_t>;
    { t.size() } -> std::convertible_to<size_t>;
    { T::continuous } -> std::convertible_to<bool>;
  } && (!T::continuous || requires(T t) {
    { t.begin } -> std::convertible_to<size_t>;
    { t.end } -> std::convertible_to<size_t>;
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
  requires(T t, size_t i) {
    { T{ i, i, i } } -> std::convertible_to<T>;
    { t.world_rank_of(i) } -> std::convertible_to<size_t>;
  };

/**
 * ExplicitContinuousPartition
 *
 * Represents a contiguous interval [begin, end) subset of {0, ..., n-1}.
 */
struct ExplicitContinuousPartition
{
  static constexpr bool continuous = true;

  size_t n     = 0;
  size_t begin = 0;
  size_t end   = 0;

  explicit constexpr ExplicitContinuousPartition() noexcept = default;

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

/**
 * CyclicPartition
 *
 * Indices owned by rank r: { i | i % world_size == r }
 */
struct CyclicPartition
{
  static constexpr bool continuous = false;

  size_t n;
  size_t world_rank;
  size_t world_size;

  constexpr CyclicPartition(size_t n, size_t world_rank, size_t world_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
  }

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

/**
 * BlockCyclicPartition
 *
 * Blocks of consecutive indices of size block_size are distributed cyclically among ranks.
 */
struct BlockCyclicPartition
{
  static constexpr bool continuous = false;

  size_t n;
  size_t world_rank;
  size_t world_size;
  size_t block_size;

  static constexpr size_t default_block_size = 512;

  constexpr BlockCyclicPartition(size_t n,
                                 size_t world_rank,
                                 size_t world_size,
                                 size_t block_size = default_block_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
    , block_size(block_size)
  {
  }

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
    return (num_full_owned_blocks_before * block_size) + offset_in_block;
  }

  // Precondition: k < size()
  [[nodiscard]] constexpr auto select(size_t k) const noexcept -> size_t
  {
    if (world_size == 1)
      return k;
    size_t const local_block        = k / block_size; // which owned block (0-based)
    size_t const offset             = k % block_size; // offset within that block
    size_t const global_block       = (local_block * world_size) + world_rank;
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
  size_t world_rank;
  size_t world_size;

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

/**
 * BalancedSlicePartition
 *
 * Contiguous slicing with size difference at most one:
 * - First (n % world_size) ranks get (base + 1) elements.
 * - Remaining ranks get base elements, where base = n / world_size.
 */
struct BalancedSlicePartition : ExplicitContinuousPartition
{
  size_t world_rank;
  size_t world_size;

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

struct KaGenPartition : ExplicitContinuousPartition
{
  size_t world_rank = 0;
  size_t world_size = 0;

  /// contains the begin and end node for each rank r as:
  /// begin = partition[2 * r]
  /// end = partition[2 * r + 1]
  U64Buffer partition{};

  bool ordered_ranks{};

  KaGenPartition()  = default;
  ~KaGenPartition() = default;

  KaGenPartition(KaGenPartition const&) = delete;
  KaGenPartition(KaGenPartition&&)      = default;

  auto operator=(KaGenPartition const&) -> KaGenPartition& = delete;
  auto operator=(KaGenPartition&&) -> KaGenPartition&      = default;

  static auto create(size_t n, size_t begin, size_t end, size_t world_rank, size_t world_size) -> Result<KaGenPartition>
  {
    ASSERT_TRY(n != 0, ASSUMPTION_ERROR);

    KaGenPartition result{};
    result.n          = n;
    result.begin      = begin;
    result.end        = end;
    result.world_rank = world_rank;
    result.world_size = world_size;

    // reserve buffer for per rank ranges
    RESULT_TRY(result.partition, U64Buffer::create(2 * result.world_size));

    // declare custom mpi datatype for a (begin, end) tuple
    using Range              = std::tuple<u64, u64>;
    MPI_Datatype MPI_RANGE_T = mpi_make_tuple_type<Range>();
    SCOPE_GUARD(mpi_free(MPI_RANGE_T)); // auto free

    // distribute ranges across all ranks
    auto const range = Range{ begin, end };
    MPI_Allgather(&range, 1, MPI_RANGE_T, result.partition.data(), 1, MPI_RANGE_T, MPI_COMM_WORLD);

    // check if ranks are ordered (begin[i] == end[i-1])
    result.ordered_ranks = true;
    for (size_t r = 0; r < result.world_size - 1; ++r) {
      auto const this_end   = result.partition[(r * 2) + 1];
      auto const next_begin = result.partition[(r + 1) * 2];
      if (this_end != next_begin) {
        result.ordered_ranks = false;
        break;
      }
    }

    return result;
  }

  [[nodiscard]] auto world_rank_of(size_t i) const -> size_t
  {
    if (ordered_ranks) {
      // approximate rank and fix with linear search

      auto r = [&] {
        if constexpr (has_uint128_t) {
          return static_cast<size_t>(
            (static_cast<u128>(i) * static_cast<u128>(world_size)) / static_cast<u128>(n));
        }
        // ReSharper disable CppDFAUnreachableCode
        // fallback to double if
        return std::ranges::min(world_size - 1, static_cast<size_t>(static_cast<double>(i) * (static_cast<double>(world_size) / static_cast<double>(n))));
        // ReSharper restore CppDFAUnreachableCode
      }();

      while (r > 0 and i < partition[r * 2])
        --r;
      while ((r + 1) < world_size and i >= partition[(r * 2) + 1])
        ++r;

      return r;
    }

    // linear search from the beginning
    for (size_t r = 0; r < world_size; ++r)
      if (i >= partition[r * 2] && i < partition[(r * 2) + 1])
        return r;

    return -1; // should be unreachable if partition and input is valid. this is undefined behaviour.
  }
};
