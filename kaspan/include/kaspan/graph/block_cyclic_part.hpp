#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/math.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class block_cyclic_part_view
{
  using repr_t = std::conditional_t<(sizeof(vertex_t) > 4), u64, u32>;

public:
  constexpr block_cyclic_part_view() noexcept = default;
  constexpr block_cyclic_part_view(
    vertex_t n,
    i32      r,
    vertex_t b) noexcept
    : n_(integral_cast<repr_t>(n))
    , world_rank_(r)
    , block_size_(integral_cast<repr_t>(b))
  {
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(n_);
  }
  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return integral_cast<vertex_t>(n_);
    if (n_ == 0) return 0;
    auto const ub         = block_size_;
    auto const ur         = integral_cast<repr_t>(world_rank_);
    auto const us         = integral_cast<repr_t>(mpi_basic::world_size);
    auto const num_blocks = ceildiv<repr_t>(n_, ub);
    if (num_blocks <= ur) return 0;
    auto const owned_blocks     = ceildiv<repr_t>(num_blocks - ur, us);
    auto const last_owned_block = ur + (owned_blocks - 1) * us;
    auto const last_block_size  = (last_owned_block == num_blocks - 1) ? n_ - last_owned_block * ub : ub;
    return integral_cast<vertex_t>((owned_blocks - 1) * ub + last_block_size);
  }
  [[nodiscard]] constexpr auto to_local(
    vertex_t i) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return i;
    auto const ui = integral_cast<repr_t>(i);
    auto const ub = block_size_;
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return integral_cast<vertex_t>(floordiv<repr_t>(ui, ub * us) * ub + remainder<repr_t>(ui, ub));
  }
  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return k;
    auto const uk = integral_cast<repr_t>(k);
    auto const ub = block_size_;
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    auto const ur = integral_cast<repr_t>(world_rank_);
    return integral_cast<vertex_t>(floordiv<repr_t>(uk, ub) * (ub * us) + (ur * ub) + remainder<repr_t>(uk, ub));
  }
  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    if (mpi_basic::world_size == 1) return true;
    auto const ui = integral_cast<repr_t>(i);
    auto const ub = block_size_;
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    auto const ur = integral_cast<repr_t>(world_rank_);
    return remainder<repr_t>(floordiv<repr_t>(ui, ub), us) == ur;
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
  repr_t block_size_ = 512;
};

class block_cyclic_part
{
  using repr_t = std::conditional_t<(sizeof(vertex_t) > 4), u64, u32>;

public:
  constexpr block_cyclic_part() noexcept = default;
  explicit constexpr block_cyclic_part(
    vertex_t n,
    vertex_t b = 512) noexcept
    : n_(integral_cast<repr_t>(n))
    , block_size_(integral_cast<repr_t>(b))
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
    vertex_t i,
    vertex_t b = 512) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    auto const ui = integral_cast<repr_t>(i);
    auto const ub = integral_cast<repr_t>(b);
    auto const us = integral_cast<repr_t>(mpi_basic::world_size);
    return integral_cast<i32>(remainder<repr_t>(floordiv<repr_t>(ui, ub), us));
  }
  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> block_cyclic_part_view
  {
    return block_cyclic_part_view{ integral_cast<vertex_t>(n_), r, integral_cast<vertex_t>(block_size_) };
  }
  [[nodiscard]] constexpr auto view() const noexcept -> block_cyclic_part_view
  {
    return block_cyclic_part_view{ integral_cast<vertex_t>(n_), mpi_basic::world_rank, integral_cast<vertex_t>(block_size_) };
  }

private:
  repr_t n_          = 0;
  repr_t block_size_ = 512;
};

} // namespace kaspan
