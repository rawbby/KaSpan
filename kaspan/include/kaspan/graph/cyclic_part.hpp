#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/math.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class cyclic_part_view
{
  using repr_t = std::conditional_t<(sizeof(vertex_t) > 4), u64, u32>;

public:
  constexpr cyclic_part_view() noexcept = default;
  constexpr cyclic_part_view(
    vertex_t n,
    i32      r) noexcept
    : n_(integral_cast<repr_t>(n))
    , world_rank_(r)
  {
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(n_);
  }
  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return integral_cast<vertex_t>(n_);
    auto const ur = integral_cast<repr_t>(world_rank_);
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return ur >= n_ ? 0 : integral_cast<vertex_t>(ceildiv<repr_t>(n_ - ur, us));
  }
  [[nodiscard]] constexpr auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return i;
    auto const ui = integral_cast<repr_t>(i);
    auto const ur = integral_cast<repr_t>(world_rank_);
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return integral_cast<vertex_t>(floordiv<repr_t>(ui - ur, us));
  }
  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return k;
    auto const uk = integral_cast<repr_t>(k);
    auto const ur = integral_cast<repr_t>(world_rank_);
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return integral_cast<vertex_t>(ur + uk * us);
  }
  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    if (mpi_basic::world_size == 1) return true;
    auto const ui = integral_cast<repr_t>(i);
    auto const ur = integral_cast<repr_t>(world_rank_);
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return remainder<repr_t>(ui, us) == ur;
  }
  [[nodiscard]] static constexpr auto continuous() noexcept -> bool
  {
    return false;
  }
  [[nodiscard]] static constexpr auto ordered() noexcept -> bool
  {
    return false;
  }
  [[nodiscard]] static constexpr auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }
  [[nodiscard]] constexpr auto world_rank() const noexcept -> i32
  {
    return world_rank_;
  }

private:
  repr_t n_          = 0;
  i32    world_rank_ = 0;
};

class cyclic_part
{
  using repr_t = std::conditional_t<(sizeof(vertex_t) > 4), u64, u32>;

public:
  constexpr cyclic_part() noexcept = default;
  explicit constexpr cyclic_part(
    vertex_t n) noexcept
    : n_(integral_cast<repr_t>(n))
  {
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(n_);
  }
  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return view().local_n();
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
  [[nodiscard]] static constexpr auto continuous() noexcept -> bool
  {
    return false;
  }
  [[nodiscard]] static constexpr auto ordered() noexcept -> bool
  {
    return false;
  }
  [[nodiscard]] static constexpr auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }
  [[nodiscard]] static constexpr auto world_rank() noexcept -> i32
  {
    return mpi_basic::world_rank;
  }
  [[nodiscard]] static constexpr auto world_rank_of(
    vertex_t i) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    auto const ui = integral_cast<repr_t>(i);
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return integral_cast<i32>(remainder<repr_t>(ui, us));
  }
  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> cyclic_part_view
  {
    return cyclic_part_view{ integral_cast<vertex_t>(n_), r };
  }
  [[nodiscard]] constexpr auto view() const noexcept -> cyclic_part_view
  {
    return cyclic_part_view{ integral_cast<vertex_t>(n_), mpi_basic::world_rank };
  }

private:
  repr_t n_ = 0;
};

} // namespace kaspan
