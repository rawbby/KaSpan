#pragma once

#include <memory/borrow.hpp>
#include <scc/base.hpp>
#include <util/arithmetic.hpp>

#include <concepts>
#include <mpi.h>

template<class T>
concept PartConcept =

  std::is_move_constructible_v<T>

  and std::is_move_assignable_v<T>

  and std::is_copy_constructible_v<T>

  and std::is_copy_assignable_v<T>

  and requires(T t, vertex_t i) {
        { t.n } -> std::convertible_to<vertex_t>;
        { t.has_local(i) } -> std::convertible_to<bool>;
        { t.to_local(i) } -> std::convertible_to<vertex_t>;
        { t.to_global(i) } -> std::convertible_to<vertex_t>;
        { t.local_n() } -> std::convertible_to<vertex_t>;
        { T::continuous } -> std::convertible_to<bool>;
        { T::ordered } -> std::convertible_to<bool>;
      }

  // if ordered then must be continuous
  and (not T::ordered or T::continuous)

  // if continuous then defines range
  and (not T::continuous or requires(T t) {
        { t.begin } -> std::convertible_to<vertex_t>;
        { t.end } -> std::convertible_to<vertex_t>;
      });

template<class T>
concept WorldPartConcept =

  PartConcept<T>

  and requires(T t, vertex_t i, i32 r) {
        { t.world_size } -> std::convertible_to<i32>;
        { t.world_rank } -> std::convertible_to<i32>;
        { t.world_rank_of(i) } -> std::convertible_to<i32>;
        { t.world_part_of(r) } -> std::same_as<T>;
      };

struct SinglePart
{
  static constexpr bool     continuous = true;
  static constexpr bool     ordered    = true;
  static constexpr vertex_t begin      = 0;

  vertex_t n   = 0;
  vertex_t end = 0;

  constexpr SinglePart() noexcept  = default;
  constexpr ~SinglePart() noexcept = default;

  explicit constexpr SinglePart(vertex_t n) noexcept
    : n(n)
    , end(n)
  {
  }

  constexpr SinglePart(SinglePart const&) noexcept = default;
  constexpr SinglePart(SinglePart&&) noexcept      = default;

  constexpr auto operator=(SinglePart const&) noexcept -> SinglePart& = default;
  constexpr auto operator=(SinglePart&&) noexcept -> SinglePart&      = default;

  [[nodiscard]] static constexpr auto has_local(vertex_t /* i */) noexcept -> bool
  {
    return true;
  }

  [[nodiscard]] static constexpr auto to_local(vertex_t i) noexcept -> vertex_t
  {
    return i;
  }

  [[nodiscard]] static constexpr auto to_global(vertex_t k) noexcept -> vertex_t
  {
    return k;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return n;
  }
};

struct SingleWorldPart
{
  static constexpr bool     continuous = true;
  static constexpr bool     ordered    = true;
  static constexpr vertex_t begin      = 0;
  static constexpr vertex_t world_rank = 0;
  static constexpr vertex_t world_size = 1;

  vertex_t n   = 0;
  vertex_t end = 0;

  constexpr SingleWorldPart() noexcept  = default;
  constexpr ~SingleWorldPart() noexcept = default;

  explicit constexpr SingleWorldPart(vertex_t n) noexcept
    : n(n)
    , end(n)
  {
  }

  constexpr SingleWorldPart(SingleWorldPart const&) noexcept = default;
  constexpr SingleWorldPart(SingleWorldPart&&) noexcept      = default;

  constexpr auto operator=(SingleWorldPart const&) noexcept -> SingleWorldPart& = default;
  constexpr auto operator=(SingleWorldPart&&) noexcept -> SingleWorldPart&      = default;

  [[nodiscard]] static constexpr auto has_local(vertex_t /* i */) noexcept -> bool
  {
    return true;
  }

  [[nodiscard]] static constexpr auto to_local(vertex_t i) noexcept -> vertex_t
  {
    return i;
  }

  [[nodiscard]] static constexpr auto to_global(vertex_t k) noexcept -> vertex_t
  {
    return k;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return n;
  }

  [[nodiscard]] static constexpr auto world_rank_of(vertex_t /* i */) noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] auto world_part_of(i32 /* r */) const -> SingleWorldPart
  {
    return SingleWorldPart{ n };
  }
};

struct ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = false;

  vertex_t n     = 0;
  vertex_t begin = 0;
  vertex_t end   = 0;

  constexpr ExplicitContinuousPart() noexcept  = default;
  constexpr ~ExplicitContinuousPart() noexcept = default;

  explicit constexpr ExplicitContinuousPart(vertex_t n) noexcept
    : n(n)
  {
  }

  constexpr ExplicitContinuousPart(vertex_t n, vertex_t begin, vertex_t end) noexcept
    : n(n)
    , begin(begin)
    , end(end)
  {
    DEBUG_ASSERT_LE(begin, end);
    DEBUG_ASSERT_LE(end, n);
  }

  constexpr ExplicitContinuousPart(ExplicitContinuousPart const&) noexcept = default;
  constexpr ExplicitContinuousPart(ExplicitContinuousPart&&) noexcept      = default;

  constexpr auto operator=(ExplicitContinuousPart const&) noexcept -> ExplicitContinuousPart& = default;
  constexpr auto operator=(ExplicitContinuousPart&&) noexcept -> ExplicitContinuousPart&      = default;

  [[nodiscard]] constexpr auto has_local(vertex_t global_u) const noexcept -> bool
  {
    return global_u >= begin and global_u < end;
  }

  [[nodiscard]] constexpr auto to_local(vertex_t global_u) const noexcept -> vertex_t
  {
    return global_u - begin;
  }

  [[nodiscard]] constexpr auto to_global(vertex_t local_k) const noexcept -> vertex_t
  {
    return begin + local_k;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return end - begin;
  }
};

struct CyclicPart final
{
  static constexpr bool continuous = false;
  static constexpr bool ordered    = false;

  vertex_t n          = 0;
  i32      world_rank = 0;
  i32      world_size = 0;

  constexpr CyclicPart() noexcept  = default;
  constexpr ~CyclicPart() noexcept = default;

  constexpr CyclicPart(vertex_t n, i32 world_rank, i32 world_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
  }

  explicit CyclicPart(vertex_t n) noexcept
    : CyclicPart(n, mpi_basic::world_rank, mpi_basic::world_size)
  {
  }

  constexpr CyclicPart(CyclicPart const&) noexcept = default;
  constexpr CyclicPart(CyclicPart&&) noexcept      = default;

  constexpr auto operator=(CyclicPart const&) noexcept -> CyclicPart& = default;
  constexpr auto operator=(CyclicPart&&) noexcept -> CyclicPart&      = default;

  [[nodiscard]] constexpr auto has_local(vertex_t i) const noexcept -> bool
  {
    return world_size == 1 || (i % world_size) == world_rank;
  }

  [[nodiscard]] constexpr auto world_rank_of(vertex_t i) const noexcept -> i32
  {
    return world_size == 1 ? 0 : (i % world_size);
  }

  [[nodiscard]] constexpr auto to_local(vertex_t i) const noexcept -> vertex_t
  {
    if (world_size == 1)
      return i;
    return (i - world_rank) / world_size;
  }

  [[nodiscard]] constexpr auto to_global(vertex_t k) const noexcept -> vertex_t
  {
    if (world_size == 1)
      return k;
    return world_rank + (k * world_size);
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    if (world_size == 1)
      return n;
    if (n <= world_rank)
      return 0;
    return ((n - 1 - world_rank) / world_size) + 1;
  }

  [[nodiscard]] auto world_part_of(i32 r) const -> CyclicPart
  {
    return CyclicPart{ n, r, world_size };
  }
};

struct BlockCyclicPart final
{
  static constexpr bool     continuous         = false;
  static constexpr bool     ordered            = false;
  static constexpr vertex_t default_block_size = 512;

  vertex_t n          = 0;
  i32      world_rank = 0;
  i32      world_size = 0;
  vertex_t block_size = 0;

  constexpr BlockCyclicPart() noexcept  = default;
  constexpr ~BlockCyclicPart() noexcept = default;

  constexpr BlockCyclicPart(vertex_t n, i32 world_rank, i32 world_size, vertex_t block_size = default_block_size) noexcept
    : n(n)
    , world_rank(world_rank)
    , world_size(world_size)
    , block_size(block_size)
  {
  }

  explicit BlockCyclicPart(vertex_t n) noexcept
    : BlockCyclicPart(n, mpi_basic::world_rank, mpi_basic::world_size)
  {
  }

  constexpr BlockCyclicPart(BlockCyclicPart const&) noexcept = default;
  constexpr BlockCyclicPart(BlockCyclicPart&&) noexcept      = default;

  constexpr auto operator=(BlockCyclicPart const&) noexcept -> BlockCyclicPart& = default;
  constexpr auto operator=(BlockCyclicPart&&) noexcept -> BlockCyclicPart&      = default;

  [[nodiscard]] constexpr auto has_local(vertex_t global) const noexcept -> bool
  {
    if (world_size == 1)
      return true;
    vertex_t const block = global / block_size;
    return block % world_size == world_rank;
  }

  [[nodiscard]] constexpr auto world_rank_of(vertex_t global) const noexcept -> i32
  {
    if (world_size == 1)
      return 0;
    vertex_t const block = global / block_size;
    return block % world_size;
  }

  // Precondition: has_local(i)
  [[nodiscard]] constexpr auto to_local(vertex_t global) const noexcept -> vertex_t
  {
    if (world_size == 1)
      return global;
    vertex_t const block                        = global / block_size;
    i32 const      offset_in_block              = global % block_size;
    vertex_t const num_full_owned_blocks_before = block / world_size;
    return (num_full_owned_blocks_before * block_size) + offset_in_block;
  }

  // Precondition: k < local_n()
  [[nodiscard]] constexpr auto to_global(vertex_t local) const noexcept -> vertex_t
  {
    if (world_size == 1)
      return local;
    vertex_t const local_block        = local / block_size; // which owned block (0-based)
    vertex_t const offset             = local % block_size; // offset within that block
    vertex_t const global_block       = (local_block * world_size) + world_rank;
    vertex_t const global_block_start = global_block * block_size;
    return global_block_start + offset;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    if (world_size == 1)
      return n;
    if (n == 0)
      return 0;

    vertex_t const num_blocks = (n + block_size - 1) / block_size; // ceil(n / block_size)
    if (num_blocks == 0)
      return 0;

    // Number of owned blocks
    vertex_t owned_blocks = 0;
    if (num_blocks > world_rank)
      owned_blocks = (num_blocks - 1 - world_rank) / world_size + 1;

    if (owned_blocks == 0)
      return 0;

    // Elements = (owned_blocks - 1) full blocks + local_n of last owned block
    vertex_t const last_owned_block       = world_rank + ((owned_blocks - 1) * world_size);
    bool const     owns_global_last_block = last_owned_block == num_blocks - 1;
    vertex_t const last_block_size        = owns_global_last_block ? n - (last_owned_block * block_size) : block_size;

    return ((owned_blocks - 1) * block_size) + last_block_size;
  }

  [[nodiscard]] auto world_part_of(i32 r) const -> BlockCyclicPart
  {
    return BlockCyclicPart{ n, r, world_size };
  }
};

struct TrivialSlicePart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = true;

  i32 world_rank = 0;
  i32 world_size = 0;

  constexpr TrivialSlicePart() noexcept  = default;
  constexpr ~TrivialSlicePart() noexcept = default;

  constexpr TrivialSlicePart(vertex_t n, i32 world_rank, i32 world_size) noexcept
    : ExplicitContinuousPart(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
    DEBUG_ASSERT_GT(world_size, 0);
    DEBUG_ASSERT_GE(world_rank, 0);
    DEBUG_ASSERT_LT(world_rank, world_size);

    if (world_size == 1) {
      begin = 0;
      end   = n;
      return;
    }

    auto const base  = n / world_size;
    auto const start = static_cast<vertex_t>(world_rank) * base;
    begin            = start;
    if (world_rank + 1 == world_size) {
      end = n;
    } else {
      end = start + base;
    }
  }

  explicit TrivialSlicePart(vertex_t n) noexcept
    : TrivialSlicePart(n, mpi_basic::world_rank, mpi_basic::world_size)
  {
  }

  constexpr TrivialSlicePart(TrivialSlicePart const&) noexcept = default;
  constexpr TrivialSlicePart(TrivialSlicePart&&) noexcept      = default;

  constexpr auto operator=(TrivialSlicePart const&) noexcept -> TrivialSlicePart& = default;
  constexpr auto operator=(TrivialSlicePart&&) noexcept -> TrivialSlicePart&      = default;

  [[nodiscard]] constexpr auto world_rank_of(vertex_t global) const noexcept -> i32
  {
    if (world_size == 1)
      return 0;

    vertex_t const base = n / world_size;
    if (base == 0)
      return world_size - 1;

    vertex_t const cutoff = base * static_cast<vertex_t>(world_size - 1);
    if (global < cutoff)
      return static_cast<i32>(global / base);

    return world_size - 1;
  }

  [[nodiscard]] auto world_part_of(i32 r) const -> TrivialSlicePart
  {
    return TrivialSlicePart{ n, r, world_size };
  }
};

struct BalancedSlicePart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = true;

  i32 world_rank = 0;
  i32 world_size = 0;

  constexpr BalancedSlicePart() noexcept  = default;
  constexpr ~BalancedSlicePart() noexcept = default;

  constexpr BalancedSlicePart(vertex_t n, i32 world_rank, i32 world_size) noexcept
    : ExplicitContinuousPart(n)
    , world_rank(world_rank)
    , world_size(world_size)
  {
    DEBUG_ASSERT_GT(world_size, 0);
    DEBUG_ASSERT_GE(world_rank, 0);
    DEBUG_ASSERT_LT(world_rank, world_size);

    if (world_size == 1) {
      begin = 0;
      end   = n;
      return;
    }

    vertex_t const base = n / world_size;
    i32 const      rem  = n % world_size;

    if (world_rank < rem) {
      begin = static_cast<vertex_t>(world_rank) * (base + 1);
      end   = begin + (base + 1);
    } else {
      begin = static_cast<vertex_t>(rem) * (base + 1) + static_cast<vertex_t>(world_rank - rem) * base;
      end   = begin + base;
    }
  }

  explicit BalancedSlicePart(vertex_t n) noexcept
    : BalancedSlicePart(n, mpi_basic::world_rank, mpi_basic::world_size)
  {
  }

  constexpr BalancedSlicePart(BalancedSlicePart const&) noexcept = default;
  constexpr BalancedSlicePart(BalancedSlicePart&&) noexcept      = default;

  constexpr auto operator=(BalancedSlicePart const&) noexcept -> BalancedSlicePart& = default;
  constexpr auto operator=(BalancedSlicePart&&) noexcept -> BalancedSlicePart&      = default;

  [[nodiscard]] constexpr auto world_rank_of(vertex_t i) const noexcept -> i32
  {
    if (world_size == 1) {
      return 0;
    }

    vertex_t const base = n / world_size;
    i32 const      rem  = n % world_size;

    if (rem == 0) {
      return base == 0 ? 0 : i / base;
    }

    vertex_t const split = (base + 1) * rem;
    if (i < split) {
      return static_cast<i32>(i / (base + 1));
    }

    return rem + static_cast<i32>((i - split) / base);
  }

  [[nodiscard]] auto world_part_of(i32 r) const -> BalancedSlicePart
  {
    return BalancedSlicePart{ n, r, world_size };
  }
};

struct ExplicitContinuousWorldPart final : ExplicitContinuousPart
{
  static constexpr bool continuous = true;
  static constexpr bool ordered    = false;

  i32       world_rank = 0;
  i32       world_size = 0;
  vertex_t* part;

  ExplicitContinuousWorldPart()  = default;
  ~ExplicitContinuousWorldPart() = default;

  ExplicitContinuousWorldPart(vertex_t n, vertex_t begin, vertex_t end, i32 world_rank, i32 world_size, void** memory)
    : world_rank(world_rank)
    , world_size(world_size)
  {
    part = ::borrow_array<vertex_t>(memory, 2 * world_size);

    auto const local_range = std::array{ begin, end };
    mpi_basic::allgather(local_range.data(), 2, part, 2);

    this->n     = n;
    this->begin = part[2 * world_rank];
    this->end   = part[2 * world_rank + 1];
  }

  ExplicitContinuousWorldPart(ExplicitContinuousWorldPart const&) = default;
  ExplicitContinuousWorldPart(ExplicitContinuousWorldPart&&)      = default;

  auto operator=(ExplicitContinuousWorldPart const&) -> ExplicitContinuousWorldPart& = default;
  auto operator=(ExplicitContinuousWorldPart&&) -> ExplicitContinuousWorldPart&      = default;

  [[nodiscard]] auto world_rank_of(vertex_t i) const -> i32
  {
    for (i32 r = 0; r < world_size; ++r) {
      if (i >= part[2 * r] and i < part[2 * r + 1]) {
        return r;
      }
    }
    return -1;
  }

  [[nodiscard]] auto world_part_of(i32 r) const -> ExplicitContinuousWorldPart
  {
    ExplicitContinuousWorldPart result;
    result.n          = n;
    result.begin      = part[r * 2];
    result.end        = part[r * 2 + 1];
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

  i32       world_rank = 0;
  i32       world_size = 0;
  vertex_t* part;

  ExplicitSortedContinuousWorldPart()  = default;
  ~ExplicitSortedContinuousWorldPart() = default;

  ExplicitSortedContinuousWorldPart(vertex_t n, vertex_t end, i32 world_rank, i32 world_size, void** memory)
    : world_rank(world_rank)
    , world_size(world_size)
  {
    part = ::borrow_array<vertex_t>(memory, world_size);

    vertex_t const local_end = end;
    mpi_basic::allgather(local_end, part);

    this->n     = n;
    this->begin = world_rank == 0 ? 0 : part[world_rank - 1];
    this->end   = part[world_rank];
  }

  ExplicitSortedContinuousWorldPart(ExplicitSortedContinuousWorldPart const&) = default;
  ExplicitSortedContinuousWorldPart(ExplicitSortedContinuousWorldPart&&)      = default;

  auto operator=(ExplicitSortedContinuousWorldPart const&) -> ExplicitSortedContinuousWorldPart& = default;
  auto operator=(ExplicitSortedContinuousWorldPart&&) -> ExplicitSortedContinuousWorldPart&      = default;

  [[nodiscard]] auto world_rank_of(vertex_t i) const -> i32
  {
    auto r = i * static_cast<u64>(world_size) / static_cast<u64>(n);
    while (r > 0 and i < part[r - 1]) {
      --r;
    }
    while (r + 1 < world_size and i >= part[r]) {
      ++r;
    }
    return r;
  }

  [[nodiscard]] auto world_part_of(i32 r) const -> ExplicitSortedContinuousWorldPart
  {
    ExplicitSortedContinuousWorldPart result;
    result.n          = n;
    result.begin      = r == 0 ? 0 : part[r - 1];
    result.end        = part[r];
    result.world_rank = r;
    result.world_size = world_size;
    result.part       = part;
    return result;
  }
};
