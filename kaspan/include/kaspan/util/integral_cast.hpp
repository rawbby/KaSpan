#pragma once

#include <kaspan/util/arithmetic.hpp>
#include <kaspan/debug/assert_in_range.hpp>

#include <type_traits>
#include <limits>
#include <utility>

namespace kaspan {

template<integral_concept T, integral_concept F>
constexpr auto
integral_cast(F&& value) -> T
{
  if (std::is_constant_evaluated()) {
    return static_cast<T>(std::forward<F>(value));
  }

  DEBUG_ASSERT_IN_RANGE_INCLUSIVE(value, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
  return static_cast<T>(std::forward<F>(value));
}

} // namespace kaspan
