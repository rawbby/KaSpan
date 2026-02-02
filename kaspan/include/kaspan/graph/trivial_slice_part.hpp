#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class trivial_slice_part_view
{
public:
  constexpr trivial_slice_part_view() noexcept = default;

  constexpr trivial_slice_part_view(
    vertex_t n,
    vertex_t local_n,
    vertex_t begin,
    vertex_t end,
    i32      world_rank) noexcept
    : n_(n)
    , local_n_(local_n)
    , begin_(begin)
    , end_(end)
    , world_rank_(world_rank)
  {
  }

  trivial_slice_part_view(
    arithmetic_concept auto n,
    arithmetic_concept auto r) noexcept
    : n_(integral_cast<vertex_t>(n))
    , world_rank_(integral_cast<i32>(r))
  {
    if (mpi_basic::world_size == 1) {
      begin_   = 0;
      end_     = n_;
      local_n_ = n_;
      return;
    }
    auto const base = n_ / mpi_basic::world_size;
    begin_          = r * base;
    end_            = (r + 1 == mpi_basic::world_size) ? n_ : begin_ + base;
    local_n_        = end_ - begin_;
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
    return integral_cast<vertex_t>(i) - begin_;
  }

  [[nodiscard]] constexpr auto to_global(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(k) + begin_;
  }

  [[nodiscard]] constexpr auto has_local(
    arithmetic_concept auto i) const noexcept -> bool
  {
    auto const j = integral_cast<vertex_t>(i);
    return j >= begin_ && j < end_;
  }

  static constexpr auto continuous = true;

  static constexpr auto ordered = true;

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
    auto const base = n_ / mpi_basic::world_size;
    if (base == 0) return mpi_basic::world_size - 1;
    auto const r = i / base;
    return (r >= mpi_basic::world_size) ? mpi_basic::world_size - 1 : integral_cast<i32>(r);
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) const noexcept -> trivial_slice_part_view
  {
    return { n_, r };
  }

  /**
   * @brief Iterate over each local index k.
   * @param consumer A function taking an index k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < local_n_; ++k)
      consumer(k);
  }

  /**
   * @brief Iterate over each local index k and its corresponding global index u.
   * @param consumer A function taking (local index k, global index u).
   */
  template<typename Consumer>
  constexpr void each_ku(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < local_n_; ++k)
      consumer(k, to_global(k));
  }

  /**
   * @brief Iterate over each global index u.
   * @param consumer A function taking global index u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < local_n_; ++k)
      consumer(to_global(k));
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

class trivial_slice_part
{
public:
  constexpr trivial_slice_part() noexcept = default;
  explicit trivial_slice_part(
    arithmetic_concept auto n) noexcept
    : n_(integral_cast<vertex_t>(n))
  {
    if (mpi_basic::world_size == 1) {
      begin_   = 0;
      end_     = n_;
      local_n_ = n_;
      return;
    }
    auto const base = n_ / mpi_basic::world_size;
    begin_          = mpi_basic::world_rank * base;
    end_            = (mpi_basic::world_rank + 1 == mpi_basic::world_size) ? n_ : begin_ + base;
    local_n_        = end_ - begin_;
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
    return integral_cast<vertex_t>(i) - begin_;
  }

  [[nodiscard]] constexpr auto to_global(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(k) + begin_;
  }

  [[nodiscard]] constexpr auto has_local(
    arithmetic_concept auto i) const noexcept -> bool
  {
    auto const j = integral_cast<vertex_t>(i);
    return j >= begin_ && j < end_;
  }

  static constexpr auto continuous = true;

  static constexpr auto ordered = true;

  [[nodiscard]] static auto world_size() noexcept -> i32
  {
    return mpi_basic::world_size;
  }

  [[nodiscard]] static auto world_rank() noexcept -> i32
  {
    return mpi_basic::world_rank;
  }

  [[nodiscard]] auto world_rank_of(
    arithmetic_concept auto i) const noexcept -> i32
  {
    auto const j = integral_cast<vertex_t>(i);
    if (mpi_basic::world_size == 1) return 0;

    auto const base = n_ / integral_cast<vertex_t>(mpi_basic::world_size);
    if (base == 0) return mpi_basic::world_size - 1;

    auto const r = j / base;
    if (r < integral_cast<vertex_t>(mpi_basic::world_size)) return integral_cast<i32>(r);

    return mpi_basic::world_size - 1;
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) const noexcept -> trivial_slice_part_view
  {
    return view().world_part_of(r);
  }

  /**
   * @brief Iterate over each local index k.
   * @param consumer A function taking an index k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    view().each_k(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local index k and its corresponding global index u.
   * @param consumer A function taking (local index k, global index u).
   */
  template<typename Consumer>
  constexpr void each_ku(
    Consumer&& consumer) const noexcept
  {
    view().each_ku(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global index u.
   * @param consumer A function taking global index u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  [[nodiscard]] auto view() const noexcept -> trivial_slice_part_view
  {
    return { n_, local_n_, begin_, end_, mpi_basic::world_rank };
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
