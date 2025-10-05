#pragma once

#include <comm/MpiBasic.hpp>
#include <util/Arithmetic.hpp>

#include <limits>

struct Edge
{
  u64 u;
  u64 v;
};

struct EdgeLess
{
  [[nodiscard]] constexpr auto operator()(Edge const& lhs, Edge const& rhs) const noexcept -> bool
  {
    return lhs.u < rhs.u or (lhs.u == rhs.u and lhs.v < rhs.v);
  }

  [[nodiscard]] static constexpr auto min_value() noexcept -> Edge
  {
    constexpr auto lo = std::numeric_limits<u64>::min();
    return Edge{ lo, lo };
  }

  [[nodiscard]] static constexpr auto max_value() noexcept -> Edge
  {
    constexpr auto hi = std::numeric_limits<u64>::max();
    return Edge{ hi, hi };
  }
};
