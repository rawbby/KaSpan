#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class balanced_slice_part_view
{
public:
  constexpr balanced_slice_part_view() noexcept = default;
  constexpr balanced_slice_part_view(
    vertex_t n,
    i32      r) noexcept
    : n_(n)
    , world_rank_(r)
  {
    if (mpi_basic::world_size == 1) {
      begin_   = 0;
      end_     = n_;
      local_n_ = n_;
      return;
    }
    auto const base = n_ / mpi_basic::world_size;
    auto const rem  = n_ % mpi_basic::world_size;
    if (r < rem) {
      begin_ = r * (base + 1);
      end_   = begin_ + (base + 1);
    } else {
      begin_ = rem * (base + 1) + (r - rem) * base;
      end_   = begin_ + base;
    }
    local_n_ = end_ - begin_;
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return local_n_;
  }

  [[nodiscard]] constexpr auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    return i - begin_;
  }

  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return k + begin_;
  }

  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    return i >= begin_ && i < end_;
  }

  [[nodiscard]] static constexpr auto continuous() noexcept -> bool
  {
    return true;
  }

  [[nodiscard]] static constexpr auto ordered() noexcept -> bool
  {
    return true;
  }

  [[nodiscard]] static constexpr auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] constexpr auto world_rank() const noexcept -> i32
  {
    return world_rank_;
  }

  [[nodiscard]] constexpr auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    auto const base  = n_ / mpi_basic::world_size;
    auto const rem   = n_ % mpi_basic::world_size;
    auto const split = rem * (base + 1);
    if (i < split) return integral_cast<i32>(i / (base + 1));
    return integral_cast<i32>(rem + (i - split) / base);
  }

  [[nodiscard]] constexpr auto begin() const noexcept -> vertex_t
  {
    return begin_;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return end_;
  }

private:
  vertex_t n_          = 0;
  vertex_t local_n_    = 0;
  vertex_t begin_      = 0;
  vertex_t end_        = 0;
  i32      world_rank_ = 0;
};

class balanced_slice_part
{
public:
  constexpr balanced_slice_part() noexcept = default;
  explicit constexpr balanced_slice_part(
    vertex_t n) noexcept
    : n_(n)
  {
    if (mpi_basic::world_size == 1) {
      begin_   = 0;
      end_     = n_;
      local_n_ = n_;
      return;
    }
    auto const base = n_ / mpi_basic::world_size;
    auto const rem  = n_ % mpi_basic::world_size;
    if (mpi_basic::world_rank < rem) {
      begin_ = mpi_basic::world_rank * (base + 1);
      end_   = begin_ + (base + 1);
    } else {
      begin_ = rem * (base + 1) + (mpi_basic::world_rank - rem) * base;
      end_   = begin_ + base;
    }
    local_n_ = end_ - begin_;
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return local_n_;
  }

  [[nodiscard]] constexpr auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    return i - begin_;
  }

  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return k + begin_;
  }

  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    return i >= begin_ && i < end_;
  }

  [[nodiscard]] static constexpr auto continuous() noexcept -> bool
  {
    return true;
  }

  [[nodiscard]] static constexpr auto ordered() noexcept -> bool
  {
    return true;
  }

  [[nodiscard]] static constexpr auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] static constexpr auto world_rank() noexcept -> i32
  {
    return mpi_basic::world_rank;
  }

  [[nodiscard]] constexpr auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    auto const base  = n_ / mpi_basic::world_size;
    auto const rem   = n_ % mpi_basic::world_size;
    auto const split = rem * (base + 1);
    if (i < split) return i / (base + 1);
    return rem + (i - split) / base;
  }

  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> balanced_slice_part_view
  {
    return { n_, r };
  }

  [[nodiscard]] constexpr auto view() const noexcept -> balanced_slice_part_view
  {
    return { n_, mpi_basic::world_rank };
  }

  [[nodiscard]] constexpr auto begin() const noexcept -> vertex_t
  {
    return begin_;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return end_;
  }

private:
  vertex_t n_       = 0;
  vertex_t local_n_ = 0;
  vertex_t begin_   = 0;
  vertex_t end_     = 0;
};

} // namespace kaspan
