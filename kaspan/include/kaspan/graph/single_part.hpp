#pragma once

#include <kaspan/graph/base.hpp>

namespace kaspan {

class single_part_view
{
public:
  constexpr single_part_view() noexcept = default;
  explicit constexpr single_part_view(
    arithmetic_concept auto n) noexcept
    : n_(integral_cast<vertex_t>(n))
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
    arithmetic_concept auto i) noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(i);
  }

  [[nodiscard]] static constexpr auto to_global(
    arithmetic_concept auto k) noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(k);
  }

  [[nodiscard]] static constexpr auto has_local(
    arithmetic_concept auto /* i */) noexcept -> bool
  {
    return true;
  }

  static constexpr auto continuous = true;

  static constexpr auto ordered = true;

  [[nodiscard]] static constexpr auto world_size() noexcept -> i32
  {
    return 1;
  }

  [[nodiscard]] static constexpr auto world_rank() noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] static constexpr auto world_rank_of(
    arithmetic_concept auto /* i */) noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] constexpr auto world_part_of(
    arithmetic_concept auto /* r */) const noexcept -> single_part_view
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
    arithmetic_concept auto n) noexcept
    : n_(integral_cast<vertex_t>(n))
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
    arithmetic_concept auto i) noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(i);
  }

  [[nodiscard]] static constexpr auto to_global(
    arithmetic_concept auto k) noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(k);
  }

  [[nodiscard]] static constexpr auto has_local(
    arithmetic_concept auto /* i */) noexcept -> bool
  {
    return true;
  }

  static constexpr auto continuous = true;

  static constexpr auto ordered = true;

  [[nodiscard]] static constexpr auto world_size() noexcept -> i32
  {
    return 1;
  }

  [[nodiscard]] static constexpr auto world_rank() noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] static constexpr auto world_rank_of(
    arithmetic_concept auto /* i */) noexcept -> i32
  {
    return 0;
  }

  [[nodiscard]] constexpr auto world_part_of(
    arithmetic_concept auto /* r */) const noexcept -> single_part_view
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
