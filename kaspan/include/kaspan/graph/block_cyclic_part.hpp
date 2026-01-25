#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class block_cyclic_part_view
{
public:
  constexpr block_cyclic_part_view() noexcept = default;
  block_cyclic_part_view(
    vertex_t n,
    i32      r,
    vertex_t b) noexcept
    : n_(n)
    , world_rank_(r)
    , block_size_(b)
  {
    if (mpi_basic::world_size == 1) {
      local_n_ = n_;
      return;
    }
    if (n_ == 0) {
      local_n_ = 0;
      return;
    }
    auto const us         = mpi_basic::world_size;
    auto const num_blocks = (n_ + block_size_ - 1) / block_size_;
    if (num_blocks <= integral_cast<vertex_t>(r)) {
      local_n_ = 0;
      return;
    }
    auto const owned_blocks     = (num_blocks - integral_cast<vertex_t>(r) + us - 1) / us;
    auto const last_owned_block = integral_cast<vertex_t>(r) + (owned_blocks - 1) * us;
    auto const last_block_size  = (last_owned_block == num_blocks - 1) ? n_ - last_owned_block * block_size_ : block_size_;
    local_n_                    = (owned_blocks - 1) * block_size_ + last_block_size;
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
    return (i / (block_size_ * mpi_basic::world_size)) * block_size_ + (i % block_size_);
  }

  [[nodiscard]] auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    if (mpi_basic::world_size == 1) return k;
    return (k / block_size_) * (block_size_ * mpi_basic::world_size) + (integral_cast<vertex_t>(world_rank_) * block_size_) + (k % block_size_);
  }

  [[nodiscard]] auto has_local(
    vertex_t i) const noexcept -> bool
  {
    if (mpi_basic::world_size == 1) return true;
    return (i / block_size_) % mpi_basic::world_size == integral_cast<vertex_t>(world_rank_);
  }

  [[nodiscard]] static constexpr auto continuous() noexcept -> bool
  {
    return false;
  }

  [[nodiscard]] static constexpr auto ordered() noexcept -> bool
  {
    return false;
  }

  [[nodiscard]] static auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] constexpr auto world_rank() const noexcept -> i32
  {
    return world_rank_;
  }

  [[nodiscard]] auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return integral_cast<i32>(i / block_size_ % mpi_basic::world_size);
  }

  [[nodiscard]] auto world_part_of(
    i32 r) const noexcept -> block_cyclic_part_view
  {
    return { n_, r, block_size_ };
  }

private:
  vertex_t n_          = 0;
  vertex_t local_n_    = 0;
  i32      world_rank_ = 0;
  vertex_t block_size_ = 512;
};

class block_cyclic_part
{
public:
  constexpr block_cyclic_part() noexcept = default;
  explicit block_cyclic_part(
    vertex_t n,
    vertex_t b = 512) noexcept
    : n_(n)
    , block_size_(b)
  {
    if (mpi_basic::world_size == 1) {
      local_n_ = n_;
      return;
    }
    if (n_ == 0) {
      local_n_ = 0;
      return;
    }
    auto const num_blocks = (n_ + block_size_ - 1) / block_size_;
    if (num_blocks <= integral_cast<vertex_t>(mpi_basic::world_rank)) {
      local_n_ = 0;
      return;
    }
    auto const owned_blocks     = (num_blocks - mpi_basic::world_rank + mpi_basic::world_size - 1) / mpi_basic::world_size;
    auto const last_owned_block = mpi_basic::world_rank + (owned_blocks - 1) * mpi_basic::world_size;
    auto const last_block_size  = (last_owned_block == num_blocks - 1) ? n_ - last_owned_block * block_size_ : block_size_;
    local_n_                    = (owned_blocks - 1) * block_size_ + last_block_size;
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

  [[nodiscard]] static constexpr auto continuous() noexcept -> bool
  {
    return false;
  }

  [[nodiscard]] static constexpr auto ordered() noexcept -> bool
  {
    return false;
  }

  [[nodiscard]] static auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] static auto world_rank() noexcept -> i32
  {
    return mpi_basic::world_rank;
  }

  [[nodiscard]] constexpr auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    return world_rank_of(i, block_size_);
  }

  [[nodiscard]] static auto world_rank_of(
    vertex_t i,
    vertex_t b) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return i / b % mpi_basic::world_size;
  }

  [[nodiscard]] auto world_part_of(
    i32 r) const noexcept -> block_cyclic_part_view
  {
    return { n_, r, block_size_ };
  }

  [[nodiscard]] auto view() const noexcept -> block_cyclic_part_view
  {
    return { n_, mpi_basic::world_rank, block_size_ };
  }

private:
  vertex_t n_          = 0;
  vertex_t local_n_    = 0;
  vertex_t block_size_ = 512;
};

} // namespace kaspan
