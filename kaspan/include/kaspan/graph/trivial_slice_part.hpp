#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/math.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class trivial_slice_part_view
{
  using repr_t = std::conditional_t<(sizeof(vertex_t) > 4), u64, u32>;

public:
  constexpr trivial_slice_part_view() noexcept = default;
  constexpr trivial_slice_part_view(
    vertex_t n,
    i32      r) noexcept
    : n_(integral_cast<repr_t>(n))
    , world_rank_(r)
  {
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    if (us == 1) {
      begin_ = 0;
      end_   = n_;
      return;
    }
    auto const ur   = integral_cast<repr_t>(r);
    auto const base = floordiv<repr_t>(n_, us);
    begin_          = ur * base;
    end_            = (ur + 1 == us) ? n_ : begin_ + base;
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(n_);
  }
  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(end_ - begin_);
  }
  [[nodiscard]] constexpr auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(integral_cast<repr_t>(i) - begin_);
  }
  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(integral_cast<repr_t>(k) + begin_);
  }
  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    auto const ui = integral_cast<repr_t>(i);
    return ui >= begin_ && ui < end_;
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
  [[nodiscard]] constexpr auto begin() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(begin_);
  }
  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(end_);
  }

private:
  repr_t n_          = 0;
  repr_t begin_      = 0;
  repr_t end_        = 0;
  i32    world_rank_ = 0;
};

class trivial_slice_part
{
  using repr_t = std::conditional_t<(sizeof(vertex_t) > 4), u64, u32>;

public:
  constexpr trivial_slice_part() noexcept = default;
  explicit constexpr trivial_slice_part(
    vertex_t n) noexcept
    : n_(integral_cast<repr_t>(n))
  {
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    if (us == 1) {
      begin_ = 0;
      end_   = n_;
      return;
    }
    auto const ur   = integral_cast<repr_t>(mpi_basic::world_rank);
    auto const base = floordiv<repr_t>(n_, us);
    begin_          = ur * base;
    end_            = (ur + 1 == us) ? n_ : begin_ + base;
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(n_);
  }
  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(end_ - begin_);
  }
  [[nodiscard]] constexpr auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(integral_cast<repr_t>(i) - begin_);
  }
  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(integral_cast<repr_t>(k) + begin_);
  }
  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    auto const ui = integral_cast<repr_t>(i);
    return ui >= begin_ && ui < end_;
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
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    if (us == 1) return 0;
    auto const base = floordiv<repr_t>(n_, us);
    if (base == 0) return integral_cast<i32>(us - 1);
    auto const r = floordiv<repr_t>(integral_cast<repr_t>(i), base);
    return (r >= us) ? integral_cast<i32>(us - 1) : integral_cast<i32>(r);
  }
  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> trivial_slice_part_view
  {
    return trivial_slice_part_view{ integral_cast<vertex_t>(n_), r };
  }
  [[nodiscard]] constexpr auto view() const noexcept -> trivial_slice_part_view
  {
    return trivial_slice_part_view{ integral_cast<vertex_t>(n_), mpi_basic::world_rank };
  }
  [[nodiscard]] constexpr auto begin() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(begin_);
  }
  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(end_);
  }

private:
  repr_t n_     = 0;
  repr_t begin_ = 0;
  repr_t end_   = 0;
};

} // namespace kaspan
