#pragma once

#include <kaspan/graph/base.hpp>

namespace kaspan {

class single_part_view
{
public:
  constexpr single_part_view() noexcept = default;
  explicit constexpr single_part_view(
    vertex_t n) noexcept
    : n_(n)
  {
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] static constexpr auto to_local(
    vertex_t i) noexcept -> vertex_t
  {
    return i;
  }

  [[nodiscard]] static constexpr auto to_global(
    vertex_t k) noexcept -> vertex_t
  {
    return k;
  }

  [[nodiscard]] static constexpr auto has_local(
    vertex_t /* i */) noexcept -> bool
  {
    return true;
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
    return 1;
  }

  [[nodiscard]] static constexpr auto world_rank() noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] static constexpr auto world_rank_of(
    vertex_t /* i */) noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] constexpr auto world_part_of(
    i32 /* r */) const noexcept -> single_part_view
  {
    return single_part_view{ n_ };
  }

  [[nodiscard]] static constexpr auto begin() noexcept -> vertex_t
  {
    return 0;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return n_;
  }

private:
  vertex_t n_ = 0;
};

class single_part
{
public:
  constexpr single_part() noexcept = default;
  explicit constexpr single_part(
    vertex_t n) noexcept
    : n_(n)
  {
  }

  [[nodiscard]] constexpr auto n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] constexpr auto local_n() const noexcept -> vertex_t
  {
    return n_;
  }

  [[nodiscard]] static constexpr auto to_local(
    vertex_t i) noexcept -> vertex_t
  {
    return i;
  }

  [[nodiscard]] static constexpr auto to_global(
    vertex_t k) noexcept -> vertex_t
  {
    return k;
  }

  [[nodiscard]] static constexpr auto has_local(
    vertex_t /* i */) noexcept -> bool
  {
    return true;
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
    return 1;
  }

  [[nodiscard]] static constexpr auto world_rank() noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] static constexpr auto world_rank_of(
    vertex_t /* i */) noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] constexpr auto world_part_of(
    i32 /* r */) const noexcept -> single_part_view
  {
    return single_part_view{ n_ };
  }

  [[nodiscard]] auto view() const noexcept -> single_part_view
  {
    return single_part_view{ n_ };
  }

  [[nodiscard]] static constexpr auto begin() noexcept -> vertex_t
  {
    return 0;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> vertex_t
  {
    return n_;
  }

private:
  vertex_t n_ = 0;
};

} // namespace kaspan
