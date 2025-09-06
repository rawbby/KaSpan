#pragma once

#include <util/Arithmetic.hpp>
#include <util/MpiTuple.hpp>

#include <limits>

using Edge = MpiTupleType<u64, u64>;

struct EdgeLess
{
  constexpr auto operator()(Edge const& lhs, Edge const& rhs) const noexcept -> bool
  {
    return mpi_tuple_lt(lhs, rhs);
  }

  static constexpr auto min_value() -> Edge
  {
    constexpr auto lo = std::numeric_limits<u64>::min();
    return Edge{ lo, lo };
  }

  static constexpr auto max_value() -> Edge
  {
    constexpr auto hi = std::numeric_limits<u64>::max();
    return Edge{ hi, hi };
  }
};
