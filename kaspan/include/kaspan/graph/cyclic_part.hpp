#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class cyclic_part_view
{
public:
  constexpr cyclic_part_view() noexcept = default;
  cyclic_part_view(
    vertex_t n,
    i32      r) noexcept
    : n_(n)
    , world_rank_(r)
  {
    if (mpi_basic::world_size == 1) {
      local_n_ = n_;
      return;
    }
    local_n_ = integral_cast<vertex_t>(r) >= n_ ? 0 : (n_ - integral_cast<vertex_t>(r) + mpi_basic::world_size - 1) / mpi_basic::world_size;
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return local_n_;
  }

  [[nodiscard]] auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return i;
    return (i - integral_cast<vertex_t>(world_rank_)) / mpi_basic::world_size;
  }

  [[nodiscard]] auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return k;
    return integral_cast<vertex_t>(world_rank_) + k * mpi_basic::world_size;
  }

  [[nodiscard]] auto has_local(
    vertex_t i) const noexcept -> bool
  {
    if (mpi_basic::world_size == 1) return true;
    return (i % mpi_basic::world_size) == integral_cast<vertex_t>(world_rank_);
  }

  static constexpr auto continuous = false;

  static constexpr auto ordered = false;

  [[nodiscard]] static auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] constexpr auto world_rank() const noexcept -> i32
  {
    return world_rank_;
  }

  [[nodiscard]] static auto world_rank_of(
    vertex_t i) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return integral_cast<i32>(i % mpi_basic::world_size);
  }

  [[nodiscard]] auto world_part_of(
    i32 r) const noexcept -> cyclic_part_view
  {
    return { n_, r };
  }

private:
  vertex_t n_          = 0;
  vertex_t local_n_    = 0;
  i32      world_rank_ = 0;
};

class cyclic_part
{
public:
  constexpr cyclic_part() noexcept = default;
  explicit cyclic_part(
    vertex_t n) noexcept
    : n_(n)
  {
    if (mpi_basic::world_size == 1) {
      local_n_ = n_;
      return;
    }
    local_n_ = mpi_basic::world_rank >= n_ ? 0 : (n_ - mpi_basic::world_rank + mpi_basic::world_size - 1) / mpi_basic::world_size;
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
    return view().to_local(i);
  }

  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return view().to_global(k);
  }

  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    return view().has_local(i);
  }

  static constexpr auto continuous = false;

  static constexpr auto ordered = false;

  [[nodiscard]] static auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] static auto world_rank() noexcept -> i32
  {
    return mpi_basic::world_rank;
  }

  [[nodiscard]] static auto world_rank_of(
    vertex_t i) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    auto const us = mpi_basic::world_size;
    return i % us;
  }

  [[nodiscard]] auto world_part_of(
    i32 r) const noexcept -> cyclic_part_view
  {
    return { n_, r };
  }

  [[nodiscard]] auto view() const noexcept -> cyclic_part_view
  {
    return { n_, mpi_basic::world_rank };
  }

private:
  vertex_t n_       = 0;
  vertex_t local_n_ = 0;
};

} // namespace kaspan
