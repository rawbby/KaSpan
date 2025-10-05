#pragma once

#include <buffer/Buffer.hpp>
#include <util/Arithmetic.hpp>

#include <concepts>
#include <cstddef>
#include <mpi.h>

template<class T>
concept PartConcept =

  std::is_move_constructible_v<T>

  and std::is_move_assignable_v<T>

  and std::is_copy_constructible_v<T>

  and std::is_copy_assignable_v<T>

  and requires(T t, size_t i) {
        { t.n } -> std::convertible_to<size_t>;
        { t.contains(i) } -> std::convertible_to<bool>;
        { t.rank(i) } -> std::convertible_to<size_t>;
        { t.select(i) } -> std::convertible_to<size_t>;
        { t.size() } -> std::convertible_to<size_t>;
        { T::continuous } -> std::convertible_to<bool>;
        { T::ordered } -> std::convertible_to<bool>;
      }

  // if ordered then must be continous
  and (not T::ordered or T::continuous)

  // if continuous then defines range
  and (not T::continuous or requires(T t) {
        { t.begin } -> std::convertible_to<size_t>;
        { t.end } -> std::convertible_to<size_t>;
      });

template<class T>
concept WorldPartConcept =

  PartConcept<T>

  and requires(T t, size_t i, size_t r) {
        { t.world_size } -> std::convertible_to<size_t>;
        { t.world_rank } -> std::convertible_to<size_t>;
        { t.world_rank_of(i) } -> std::convertible_to<size_t>;
        { t.world_part_of(r) } -> std::same_as<T>;
      };

struct ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = false;

  size_t n     = 0;
  size_t begin = 0;
  size_t end   = 0;

  constexpr ExplicitContinuousPart() noexcept  = default;
  constexpr ~ExplicitContinuousPart() noexcept = default;

  explicit constexpr ExplicitContinuousPart(size_t n) noexcept
    : n(n)
  {
  }

  constexpr ExplicitContinuousPart(size_t n, size_t begin, size_t end) noexcept
    : n(n)
    , begin(begin)
    , end(end)
  {
  }

  constexpr ExplicitContinuousPart(ExplicitContinuousPart const&) noexcept = default;
  constexpr ExplicitContinuousPart(ExplicitContinuousPart&&) noexcept      = default;

  constexpr auto operator=(ExplicitContinuousPart const&) noexcept -> ExplicitContinuousPart& = default;
  constexpr auto operator=(ExplicitContinuousPart&&) noexcept -> ExplicitContinuousPart&      = default;

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

struct CyclicPart final
{
  static constexpr bool continuous = false;
  static constexpr bool ordered    = false;

  size_t n          = 0;
  size_t world_rank = 0;
  size_t world_size = 0;

  constexpr CyclicPart() noexcept  = default;
  constexpr ~CyclicPart() noexcept = default;

  constexpr CyclicPart(size_t n, size_t world_rank, size_t world_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
  }

  constexpr CyclicPart(CyclicPart const&) noexcept = default;
  constexpr CyclicPart(CyclicPart&&) noexcept      = default;

  constexpr auto operator=(CyclicPart const&) noexcept -> CyclicPart& = default;
  constexpr auto operator=(CyclicPart&&) noexcept -> CyclicPart&      = default;

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

  [[nodiscard]] auto world_part_of(size_t r) const -> CyclicPart
  {
    return CyclicPart{ n, r, world_size };
  }
};

struct BlockCyclicPart final
{
  static constexpr bool   continuous         = false;
  static constexpr bool   ordered            = false;
  static constexpr size_t default_block_size = 512;

  size_t n          = 0;
  size_t world_rank = 0;
  size_t world_size = 0;
  size_t block_size = 0;

  constexpr BlockCyclicPart() noexcept  = default;
  constexpr ~BlockCyclicPart() noexcept = default;

  constexpr BlockCyclicPart(size_t n, size_t world_rank, size_t world_size, size_t block_size = default_block_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
    , block_size(block_size)
  {
  }

  constexpr BlockCyclicPart(BlockCyclicPart const&) noexcept = default;
  constexpr BlockCyclicPart(BlockCyclicPart&&) noexcept      = default;

  constexpr auto operator=(BlockCyclicPart const&) noexcept -> BlockCyclicPart& = default;
  constexpr auto operator=(BlockCyclicPart&&) noexcept -> BlockCyclicPart&      = default;

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

  [[nodiscard]] auto world_part_of(size_t r) const -> BlockCyclicPart
  {
    return BlockCyclicPart{ n, r, world_size };
  }
};

struct TrivialSlicePart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = true;

  size_t world_rank = 0;
  size_t world_size = 0;

  constexpr TrivialSlicePart() noexcept  = default;
  constexpr ~TrivialSlicePart() noexcept = default;

  constexpr TrivialSlicePart(size_t n, size_t world_rank, size_t world_size) noexcept
    : ExplicitContinuousPart(n)
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

  constexpr TrivialSlicePart(TrivialSlicePart const&) noexcept = default;
  constexpr TrivialSlicePart(TrivialSlicePart&&) noexcept      = default;

  constexpr auto operator=(TrivialSlicePart const&) noexcept -> TrivialSlicePart& = default;
  constexpr auto operator=(TrivialSlicePart&&) noexcept -> TrivialSlicePart&      = default;

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

  [[nodiscard]] auto world_part_of(size_t r) const -> TrivialSlicePart
  {
    return TrivialSlicePart{ n, r, world_size };
  }
};

struct BalancedSlicePart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = true;

  size_t world_rank = 0;
  size_t world_size = 0;

  constexpr BalancedSlicePart() noexcept  = default;
  constexpr ~BalancedSlicePart() noexcept = default;

  constexpr BalancedSlicePart(size_t n, size_t world_rank, size_t world_size) noexcept
    : ExplicitContinuousPart(n)
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

  constexpr BalancedSlicePart(BalancedSlicePart const&) noexcept = default;
  constexpr BalancedSlicePart(BalancedSlicePart&&) noexcept      = default;

  constexpr auto operator=(BalancedSlicePart const&) noexcept -> BalancedSlicePart& = default;
  constexpr auto operator=(BalancedSlicePart&&) noexcept -> BalancedSlicePart&      = default;

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

  [[nodiscard]] auto world_part_of(size_t r) const -> BalancedSlicePart
  {
    return BalancedSlicePart{ n, r, world_size };
  }
};

struct ExplicitContinuousWorldPart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = false;

  size_t                           world_rank = 0;
  size_t                           world_size = 0;
  std::shared_ptr<U64Buffer const> part;

  ExplicitContinuousWorldPart()  = default;
  ~ExplicitContinuousWorldPart() = default;

  ExplicitContinuousWorldPart(ExplicitContinuousWorldPart const&) = default;
  ExplicitContinuousWorldPart(ExplicitContinuousWorldPart&&)      = default;

  auto operator=(ExplicitContinuousWorldPart const&) -> ExplicitContinuousWorldPart& = default;
  auto operator=(ExplicitContinuousWorldPart&&) -> ExplicitContinuousWorldPart&      = default;

  static auto create(size_t n, size_t begin, size_t end, size_t world_rank, size_t world_size) -> Result<ExplicitContinuousWorldPart>
  {

    RESULT_TRY(U64Buffer part, U64Buffer::zeroes(2 * world_size));
    auto const local_range = std::array{ begin, end };
    MPI_Allgather(&local_range, 2, MPI_UINT64_T, part.data(), 2, MPI_UINT64_T, MPI_COMM_WORLD);

    ExplicitContinuousWorldPart result{};
    result.n          = n;
    result.begin      = begin;
    result.end        = end;
    result.world_rank = world_rank;
    result.world_size = world_size;
    result.part       = std::make_shared<U64Buffer>(std::move(part));
    return result;
  }

  [[nodiscard]] auto world_rank_of(size_t i) const -> size_t
  {
    for (size_t r = 0; r < world_size; ++r)
      if (i >= part->get(r * 2) and i < part->get(r * 2 + 1))
        return r;
    return -1;
  }

  [[nodiscard]] auto world_part_of(size_t r) const -> ExplicitContinuousWorldPart
  {
    ExplicitContinuousWorldPart result;
    result.n          = n;
    result.begin      = part->get(r * 2);
    result.end        = part->get(r * 2 + 1);
    result.world_rank = r;
    result.world_size = world_size;
    result.part       = part;
    return result;
  }
};

struct ExplicitSortedContinuousWorldPart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = true;

  size_t                           world_rank = 0;
  size_t                           world_size = 0;
  std::shared_ptr<U64Buffer const> part;

  ExplicitSortedContinuousWorldPart()  = default;
  ~ExplicitSortedContinuousWorldPart() = default;

  ExplicitSortedContinuousWorldPart(ExplicitSortedContinuousWorldPart const&) = default;
  ExplicitSortedContinuousWorldPart(ExplicitSortedContinuousWorldPart&&)      = default;

  auto operator=(ExplicitSortedContinuousWorldPart const&) -> ExplicitSortedContinuousWorldPart& = default;
  auto operator=(ExplicitSortedContinuousWorldPart&&) -> ExplicitSortedContinuousWorldPart&      = default;

  static auto create(size_t n, size_t begin, size_t end, size_t world_rank, size_t world_size) -> Result<ExplicitSortedContinuousWorldPart>
  {
    RESULT_TRY(U64Buffer part, U64Buffer::zeroes(world_size));
    u64 const local_end = end;
    MPI_Allgather(&local_end, 1, MPI_UINT64_T, part.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);

    ExplicitSortedContinuousWorldPart result{};
    result.n          = n;
    result.begin      = world_rank == 0 ? 0 : part.get(world_rank - 1);
    result.end        = part.get(world_rank);
    result.world_rank = world_rank;
    result.world_size = world_size;
    result.part       = std::make_shared<U64Buffer>(std::move(part));
    return result;
  }

  [[nodiscard]] auto world_rank_of(size_t i) const -> size_t
  {
    auto r = i * static_cast<u64>(world_size) / static_cast<u64>(n);
    while (r > 0 and i < part->get(r - 1))
      --r;
    while (r + 1 < world_size and i >= part->get(r))
      ++r;
    return r;
  }

  [[nodiscard]] auto world_part_of(size_t r) const -> ExplicitSortedContinuousWorldPart
  {
    ExplicitSortedContinuousWorldPart result;
    result.n          = n;
    result.begin      = r == 0 ? 0 : part->get(r - 1);
    result.end        = part->get(r);
    result.world_rank = r;
    result.world_size = world_size;
    result.part       = part;
    return result;
  }
};
