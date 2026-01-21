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
    i32             r,
    vertex_t const* p) noexcept
    : n_(n)
    , world_rank_(r)
    , part_(p)
  {
    begin_   = part_[2 * r];
    end_     = part_[2 * r + 1];
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

  [[nodiscard]] constexpr auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    for (i32 r = 0; r < mpi_basic::world_size; ++r)
      if (i >= part_[2 * r] && i < part_[2 * r + 1]) return r;
    return -1;
  }

  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> explicit_continuous_part_view
  {
    return { n_, r, part_ };
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
    vertex_t n,
    vertex_t b,
    vertex_t e)
    : n_(n)
    , part_(line_alloc<vertex_t>(2 * mpi_basic::world_size))
  {
    vertex_t const local_range[2] = { b, e };
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

  [[nodiscard]] constexpr auto world_rank_of(
    vertex_t i) const noexcept -> i32
  {
    for (i32 r = 0; r < mpi_basic::world_size; ++r)
      if (i >= part_[2 * r] && i < part_[2 * r + 1]) return r;
    return -1;
  }

  [[nodiscard]] constexpr auto world_part_of(
    i32 r) const noexcept -> explicit_continuous_part_view
  {
    return { n_, r, part_ };
  }

  [[nodiscard]] constexpr auto view() const noexcept -> explicit_continuous_part_view
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
