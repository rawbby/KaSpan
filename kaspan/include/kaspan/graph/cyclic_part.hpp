#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class cyclic_part_view
{
public:
  constexpr cyclic_part_view() noexcept = default;
  cyclic_part_view(
    arithmetic_concept auto n,
    arithmetic_concept auto r) noexcept
    : n_(integral_cast<vertex_t>(n))
    , world_rank_(integral_cast<i32>(r))
  {
    if (mpi_basic::world_size == 1) {
      local_n_ = n_;
      return;
    }
    local_n_ = integral_cast<vertex_t>(world_rank_) >= n_ ? 0 : (n_ - integral_cast<vertex_t>(world_rank_) + mpi_basic::world_size - 1) / mpi_basic::world_size;
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
    arithmetic_concept auto i) const noexcept -> vertex_t
  {
    auto const j = integral_cast<vertex_t>(i);
    if (mpi_basic::world_size == 1) return j;
    return (j - integral_cast<vertex_t>(world_rank_)) / mpi_basic::world_size;
  }

  [[nodiscard]] auto to_global(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    auto const l = integral_cast<vertex_t>(k);
    if (mpi_basic::world_size == 1) return l;
    return integral_cast<vertex_t>(world_rank_) + l * mpi_basic::world_size;
  }

  [[nodiscard]] auto has_local(
    arithmetic_concept auto i) const noexcept -> bool
  {
    auto const j = integral_cast<vertex_t>(i);
    if (mpi_basic::world_size == 1) return true;
    return (j % mpi_basic::world_size) == integral_cast<vertex_t>(world_rank_);
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
    arithmetic_concept auto i) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return integral_cast<i32>(integral_cast<vertex_t>(i) % mpi_basic::world_size);
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) const noexcept -> cyclic_part_view
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
    arithmetic_concept auto n) noexcept
    : n_(integral_cast<vertex_t>(n))
  {
    if (mpi_basic::world_size == 1) {
      local_n_ = n_;
      return;
    }
    local_n_ = integral_cast<vertex_t>(mpi_basic::world_rank) >= n_ ? 0 : (n_ - integral_cast<vertex_t>(mpi_basic::world_rank) + mpi_basic::world_size - 1) / mpi_basic::world_size;
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
    arithmetic_concept auto i) const noexcept -> vertex_t
  {
    return view().to_local(i);
  }

  [[nodiscard]] constexpr auto to_global(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    return view().to_global(k);
  }

  [[nodiscard]] constexpr auto has_local(
    arithmetic_concept auto i) const noexcept -> bool
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
    arithmetic_concept auto i) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return integral_cast<i32>(integral_cast<vertex_t>(i) % mpi_basic::world_size);
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) const noexcept -> cyclic_part_view
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
