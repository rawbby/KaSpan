#pragma once

#include <kaspan/graph/base.hpp>

namespace kaspan {

class single_part_view
{
public:
  constexpr single_part_view() noexcept = default;

  constexpr single_part_view(
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

  /**
   * @brief Iterate over each local index k.
   * @param consumer A function taking an index k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < n_; ++k)
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
    for (vertex_t k = 0; k < n_; ++k)
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
    for (vertex_t k = 0; k < n_; ++k)
      consumer(to_global(k));
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
    return view().world_part_of(0);
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
