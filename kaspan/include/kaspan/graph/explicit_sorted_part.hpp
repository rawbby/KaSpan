#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class explicit_sorted_part_view
{
public:
  constexpr explicit_sorted_part_view() noexcept = default;

  constexpr explicit_sorted_part_view(
    vertex_t        n,
    vertex_t        local_n,
    vertex_t        begin,
    vertex_t        end,
    i32             world_rank,
    vertex_t const* part) noexcept
    : n_(n)
    , local_n_(local_n)
    , begin_(begin)
    , end_(end)
    , world_rank_(world_rank)
    , part_(part)
  {
  }

  constexpr explicit_sorted_part_view(
    arithmetic_concept auto n,
    arithmetic_concept auto r,
    vertex_t const*         p) noexcept
    : n_(integral_cast<vertex_t>(n))
    , world_rank_(integral_cast<i32>(r))
    , part_(p)
  {
    begin_   = world_rank_ == 0 ? 0 : part_[world_rank_ - 1];
    end_     = part_[world_rank_];
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
    arithmetic_concept auto i) const noexcept -> i32
  {
    auto const j = integral_cast<vertex_t>(i);
    DEBUG_ASSERT_IN_RANGE(j, 0, n());

    // initial guess based on
    // a trivial slice partition
    auto r = j * mpi_basic::world_size / n_;

    if (r > 0 && j < part_[r - 1]) {
      // downwards correction
      --r;
      while (r > 0 && j < part_[r - 1])
        --r;
    } else {
      // upwards correction
      while (r < mpi_basic::world_size - 1 && j >= part_[r])
        ++r;
    }

    return integral_cast<i32>(r);
  }

  [[nodiscard]] constexpr auto world_part_of(
    arithmetic_concept auto r) const noexcept -> explicit_sorted_part_view
  {
    return { n_, r, part_ };
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

  [[nodiscard]] constexpr auto begin() const noexcept -> vertex_t
  {
    return begin_;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return end_;
  }

private:
  vertex_t        n_          = 0;
  vertex_t        local_n_    = 0;
  vertex_t        begin_      = 0;
  vertex_t        end_        = 0;
  i32             world_rank_ = 0;
  vertex_t const* part_       = nullptr;
};

class explicit_sorted_part
{
public:
  explicit_sorted_part() noexcept = default;
  ~explicit_sorted_part() noexcept
  {
    line_free(part_);
  }

  explicit_sorted_part(
    arithmetic_concept auto n,
    arithmetic_concept auto e)
    : n_(integral_cast<vertex_t>(n))
    , part_(line_alloc<vertex_t>(mpi_basic::world_size))
  {
    mpi_basic::allgather(integral_cast<vertex_t>(e), part_);
    begin_   = mpi_basic::world_rank == 0 ? 0 : part_[mpi_basic::world_rank - 1];
    end_     = part_[mpi_basic::world_rank];
    local_n_ = end_ - begin_;
  }

  explicit_sorted_part(explicit_sorted_part const&) = delete;
  explicit_sorted_part(
    explicit_sorted_part&& other) noexcept
    : n_(other.n_)
    , local_n_(other.local_n_)
    , begin_(other.begin_)
    , end_(other.end_)
    , part_(other.part_)
  {
    other.part_ = nullptr;
  }

  auto operator=(explicit_sorted_part const&) -> explicit_sorted_part& = delete;
  auto operator=(
    explicit_sorted_part&& other) noexcept -> explicit_sorted_part&
  {
    if (this != &other) {
      line_free(part_);
      n_          = other.n_;
      local_n_    = other.local_n_;
      begin_      = other.begin_;
      end_        = other.end_;
      part_       = other.part_;
      other.part_ = nullptr;
    }
    return *this;
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
    DEBUG_ASSERT_IN_RANGE(j, 0, n());

    // initial guess based on
    // a trivial slice partition
    auto r = j * mpi_basic::world_size / n_;

    if (r > 0 && j < part_[r - 1]) {
      // downwards correction
      --r;
      while (r > 0 && j < part_[r - 1])
        --r;
    } else {
      // upwards correction
      while (r < mpi_basic::world_size - 1 && j >= part_[r])
        ++r;
    }

    return integral_cast<i32>(r);
  }

  [[nodiscard]] constexpr auto world_part_of(
    arithmetic_concept auto r) const noexcept -> explicit_sorted_part_view
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

  [[nodiscard]] auto view() const noexcept -> explicit_sorted_part_view
  {
    return { n_, local_n_, begin_, end_, mpi_basic::world_rank, part_ };
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
  vertex_t  n_       = 0;
  vertex_t  local_n_ = 0;
  vertex_t  begin_   = 0;
  vertex_t  end_     = 0;
  vertex_t* part_    = nullptr;
};

} // namespace kaspan
