#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class block_cyclic_part_view
{
public:
  constexpr block_cyclic_part_view() noexcept = default;
  block_cyclic_part_view(
    arithmetic_concept auto n,
    arithmetic_concept auto r,
    arithmetic_concept auto b = 512) noexcept
    : n_(integral_cast<vertex_t>(n))
    , world_rank_(integral_cast<i32>(r))
    , block_size_(integral_cast<vertex_t>(b))
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
    if (num_blocks <= integral_cast<vertex_t>(world_rank_)) {
      local_n_ = 0;
      return;
    }
    auto const owned_blocks     = (num_blocks - integral_cast<vertex_t>(world_rank_) + us - 1) / us;
    auto const last_owned_block = integral_cast<vertex_t>(world_rank_) + (owned_blocks - 1) * us;
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
    arithmetic_concept auto i) const noexcept -> vertex_t
  {
    auto const j = integral_cast<vertex_t>(i);
    if (mpi_basic::world_size == 1) return j;
    return (j / (block_size_ * mpi_basic::world_size)) * block_size_ + (j % block_size_);
  }

  [[nodiscard]] auto to_global(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    auto const l = integral_cast<vertex_t>(k);
    if (mpi_basic::world_size == 1) return l;
    return (l / block_size_) * (block_size_ * mpi_basic::world_size) + (integral_cast<vertex_t>(world_rank_) * block_size_) + (l % block_size_);
  }

  [[nodiscard]] auto has_local(
    arithmetic_concept auto i) const noexcept -> bool
  {
    auto const j = integral_cast<vertex_t>(i);
    if (mpi_basic::world_size == 1) return true;
    return (j / block_size_) % mpi_basic::world_size == integral_cast<vertex_t>(world_rank_);
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

  [[nodiscard]] auto world_rank_of(
    arithmetic_concept auto i) const noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return integral_cast<i32>(integral_cast<vertex_t>(i) / block_size_ % mpi_basic::world_size);
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) const noexcept -> block_cyclic_part_view
  {
    return { n_, r, block_size_ };
  }

  /**
   * @brief Iterate over each local index k.
   * @param consumer A function taking an index k.
   */
  constexpr void each_k(
    auto&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < local_n_; ++k)
      consumer(k);
  }

  /**
   * @brief Iterate over each local index k and its corresponding global index u.
   * @param consumer A function taking (local index k, global index u).
   */
  constexpr void each_ku(
    auto&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < local_n_; ++k)
      consumer(k, to_global(k));
  }

  /**
   * @brief Iterate over each global index u.
   * @param consumer A function taking global index u.
   */
  constexpr void each_u(
    auto&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < local_n_; ++k)
      consumer(to_global(k));
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
    arithmetic_concept auto n,
    arithmetic_concept auto b) noexcept
    : n_(integral_cast<vertex_t>(n))
    , block_size_(integral_cast<vertex_t>(b))
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
    auto const owned_blocks     = (num_blocks - integral_cast<vertex_t>(mpi_basic::world_rank) + mpi_basic::world_size - 1) / mpi_basic::world_size;
    auto const last_owned_block = integral_cast<vertex_t>(mpi_basic::world_rank) + (owned_blocks - 1) * mpi_basic::world_size;
    auto const last_block_size  = (last_owned_block == num_blocks - 1) ? n_ - last_owned_block * block_size_ : block_size_;
    local_n_                    = (owned_blocks - 1) * block_size_ + last_block_size;
  }
  explicit block_cyclic_part(
    arithmetic_concept auto n) noexcept
    : block_cyclic_part(n,
                        512)
  {
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

  [[nodiscard]] constexpr auto world_rank_of(
    arithmetic_concept auto i) const noexcept -> i32
  {
    return world_rank_of(i, block_size_);
  }

  [[nodiscard]] static auto world_rank_of(
    arithmetic_concept auto i,
    arithmetic_concept auto b) noexcept -> i32
  {
    if (mpi_basic::world_size == 1) return 0;
    return integral_cast<i32>(integral_cast<vertex_t>(i) / integral_cast<vertex_t>(b) % mpi_basic::world_size);
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) const noexcept -> block_cyclic_part_view
  {
    return view().world_part_of(r);
  }

  /**
   * @brief Iterate over each local index k.
   * @param consumer A function taking an index k.
   */
  constexpr void each_k(
    auto&& consumer) const noexcept
  {
    view().each_k(std::forward<decltype(consumer)>(consumer));
  }

  /**
   * @brief Iterate over each local index k and its corresponding global index u.
   * @param consumer A function taking (local index k, global index u).
   */
  constexpr void each_ku(
    auto&& consumer) const noexcept
  {
    view().each_ku(std::forward<decltype(consumer)>(consumer));
  }

  /**
   * @brief Iterate over each global index u.
   * @param consumer A function taking global index u.
   */
  constexpr void each_u(
    auto&& consumer) const noexcept
  {
    view().each_u(std::forward<decltype(consumer)>(consumer));
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
