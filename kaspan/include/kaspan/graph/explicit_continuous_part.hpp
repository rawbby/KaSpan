#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

class explicit_continuous_part_view
{
public:
  constexpr explicit_continuous_part_view() noexcept = default;

  constexpr explicit_continuous_part_view(
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

  constexpr explicit_continuous_part_view(
    arithmetic_concept auto n,
    arithmetic_concept auto r,
    vertex_t const*         p) noexcept
    : n_(integral_cast<vertex_t>(n))
    , world_rank_(integral_cast<i32>(r))
    , part_(p)
  {
    begin_   = part_[2 * world_rank_];
    end_     = part_[2 * world_rank_ + 1];
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
    auto const j = integral_cast<vertex_t>(i);
    for (i32 r = 0; r < mpi_basic::world_size; ++r)
      if (j >= part_[2 * r] && j < part_[2 * r + 1]) return r;
    return -1;
  }

  [[nodiscard]] auto world_part_of(
    arithmetic_concept auto r) noexcept -> explicit_continuous_part_view
  {
    return { n_, r, part_ };
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
  vertex_t        n_          = 0;
  vertex_t        local_n_    = 0;
  vertex_t        begin_      = 0;
  vertex_t        end_        = 0;
  i32             world_rank_ = 0;
  vertex_t const* part_       = nullptr;
};

class explicit_continuous_part
{
public:
  explicit_continuous_part() noexcept = default;
  ~explicit_continuous_part() noexcept
  {
    line_free(part_);
  }

  explicit_continuous_part(
    arithmetic_concept auto n,
    arithmetic_concept auto b,
    arithmetic_concept auto e)
    : n_(integral_cast<vertex_t>(n))
    , part_(line_alloc<vertex_t>(2 * mpi_basic::world_size))
  {
    vertex_t const local_range[2] = { integral_cast<vertex_t>(b), integral_cast<vertex_t>(e) };
    mpi_basic::allgather(local_range, 2, part_, 2);
    begin_   = part_[2 * mpi_basic::world_rank];
    end_     = part_[2 * mpi_basic::world_rank + 1];
    local_n_ = end_ - begin_;
  }

  explicit_continuous_part(explicit_continuous_part const&) = delete;
  explicit_continuous_part(
    explicit_continuous_part&& other) noexcept
    : n_(other.n_)
    , local_n_(other.local_n_)
    , begin_(other.begin_)
    , end_(other.end_)
    , part_(other.part_)
  {
    other.part_ = nullptr;
  }

  auto operator=(explicit_continuous_part const&) -> explicit_continuous_part& = delete;
  auto operator=(
    explicit_continuous_part&& other) noexcept -> explicit_continuous_part&
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

  static constexpr auto ordered = false;

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
    for (i32 r = 0; r < mpi_basic::world_size; ++r)
      if (j >= part_[2 * r] && j < part_[2 * r + 1]) return r;
    return -1;
  }

  [[nodiscard]] constexpr auto world_part_of(
    arithmetic_concept auto r) const noexcept -> explicit_continuous_part_view
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

  [[nodiscard]] auto view() const noexcept -> explicit_continuous_part_view
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
