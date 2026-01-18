#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <parallel/algorithm>

namespace kaspan {

class explicit_sorted_part_view
{
public:
  constexpr explicit_sorted_part_view() noexcept = default;
  constexpr explicit_sorted_part_view(
    vertex_t        n,
    i32             r,
    vertex_t const* p) noexcept
    : n_(n)
    , world_rank_(r)
    , part_(p)
  {
    begin_   = r == 0 ? 0 : part_[r - 1];
    end_     = part_[r];
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
    vertex_t i) const noexcept -> vertex_t
  {
    return i - begin_;
  }

  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return k + begin_;
  }

  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    return i >= begin_ && i < end_;
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

  [[nodiscard]] constexpr auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    DEBUG_ASSERT_IN_RANGE(i, 0, n());

    // initial guess based on
    // a trivial slice partition
    auto r = i * mpi_basic::world_size / n_;

    if (r > 0 && i < part_[r - 1]) {
      // downwards correction
      --r;
      while (r > 0 && i < part_[r - 1])
        --r;
    } else {
      // upwards correction
      while (r < mpi_basic::world_size - 1 && i >= part_[r])
        ++r;
    }

    return static_cast<i32>(r);
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
    vertex_t n,
    vertex_t e)
    : n_(n)
    , part_(line_alloc<vertex_t>(mpi_basic::world_size))
  {
    mpi_basic::allgather(e, part_);
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
    vertex_t i) const noexcept -> vertex_t
  {
    return i - begin_;
  }

  [[nodiscard]] constexpr auto to_global(
    vertex_t k) const noexcept -> vertex_t
  {
    return k + begin_;
  }

  [[nodiscard]] constexpr auto has_local(
    vertex_t i) const noexcept -> bool
  {
    return i >= begin_ && i < end_;
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
    DEBUG_ASSERT_IN_RANGE(i, 0, n());

    // initial guess based on
    // a trivial slice partition
    auto r = i * mpi_basic::world_size / n_;

    if (r > 0 && i < part_[r - 1]) {
      // downwards correction
      --r;
      while (r > 0 && i < part_[r - 1])
        --r;
    } else {
      // upwards correction
      while (r < mpi_basic::world_size - 1 && i >= part_[r])
        ++r;
    }

    return r;
  }

  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> explicit_sorted_part_view
  {
    return { n_, r, part_ };
  }

  [[nodiscard]] constexpr auto view() const noexcept -> explicit_sorted_part_view
  {
    return { n_, mpi_basic::world_rank, part_ };
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
