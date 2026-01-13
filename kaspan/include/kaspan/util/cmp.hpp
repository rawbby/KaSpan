#pragma once

#include <kaspan/util/arithmetic.hpp>

#include <type_traits>

namespace kaspan {

template<arithmetic_concept T, arithmetic_concept U>
constexpr bool
cmp_equal(T t, U u) noexcept
{
  if constexpr (signed_concept<T> == signed_concept<U>)
    return t == u;
  else if constexpr (signed_concept<T>)
    return t >= 0 && std::make_unsigned_t<T>(t) == u;
  else
    return u >= 0 && std::make_unsigned_t<U>(u) == t;
}

template<arithmetic_concept T, arithmetic_concept U>
constexpr bool
cmp_less(T t, U u) noexcept
{
  if constexpr (signed_concept<T> == signed_concept<U>)
    return t < u;
  else if constexpr (signed_concept<T>)
    return t < 0 || std::make_unsigned_t<T>(t) < u;
  else
    return u >= 0 && t < std::make_unsigned_t<U>(u);
}

constexpr bool
cmp_not_equal(arithmetic_concept auto t, arithmetic_concept auto u) noexcept
{
  return !cmp_equal(t, u);
}

constexpr bool
cmp_greater(arithmetic_concept auto t, arithmetic_concept auto u) noexcept
{
  return cmp_less(u, t);
}

constexpr bool
cmp_less_equal(arithmetic_concept auto t, arithmetic_concept auto u) noexcept
{
  return !cmp_less(u, t);
}

constexpr bool
cmp_greater_equal(arithmetic_concept auto t, arithmetic_concept auto u) noexcept
{
  return !cmp_less(t, u);
}

} // namespace kaspan
