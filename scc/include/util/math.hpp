#pragma once

#include <debug/assert.hpp>
#include <util/arithmetic.hpp>

#include <bit>

template<UnsignedConcept T>
constexpr auto
floordiv(T x, T base) noexcept -> T
{
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_GT(base, 0);
  }
  return x / base;
}

template<UnsignedConcept T>
constexpr auto
ceildiv(T x, T base) noexcept -> T
{
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_GT(base, 0);
    DEBUG_ASSERT_LE(x, std::numeric_limits<T>::max() - base + 1);
  }
  return (base - 1 + x) / base;
}

template<UnsignedConcept T>
constexpr auto
remainder(T x, T base) noexcept -> T
{
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_GT(base, 0);
  }
  return x % base;
}

template<UnsignedConcept T>
constexpr auto
round_down(T x, T base) noexcept -> T
{
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_GT(base, 0);
  }
  return x - (x % base);
}

template<UnsignedConcept T>
constexpr auto
round_up(T x, T base) noexcept -> T
{
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_GT(base, 0);
    DEBUG_ASSERT_LE(x, std::numeric_limits<T>::max() - base + 1);
  }
  T const r = x % base;
  return r ? x + (base - r) : x;
}

template<UnsignedConcept T>
constexpr T
clip(T x, T lo, T hi) noexcept
{
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_LE(lo, hi);
  }
  return x < lo ? lo : (x > hi ? hi : x);
}

template<u64 base, UnsignedConcept T>
constexpr auto
floordiv(T x) noexcept -> T
{
  static_assert(base > 0);
  if constexpr (base == 1) {
    return x;
  } else if constexpr (std::popcount(base) == 1) {
    constexpr auto rshift = std::countr_zero(base);
    return x >> rshift;
  } else {
    return x / static_cast<T>(base);
  }
}

template<u64 base, UnsignedConcept T>
constexpr auto
ceildiv(T x) noexcept -> T
{
  static_assert(base > 0);
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_LE(x, std::numeric_limits<T>::max() - base + 1);
  }
  if constexpr (base == 1) {
    return x;
  } else {
    return floordiv<base>(static_cast<T>(base - 1) + x);
  }
}

template<u64 base, UnsignedConcept T>
constexpr auto
remainder(T x) noexcept -> T
{
  static_assert(base > 0);
  if constexpr (base == 1) {
    return 0;
  } else if constexpr (std::popcount(base) == 1) {
    constexpr auto mask = base - 1;
    return x & mask;
  } else {
    return x % static_cast<T>(base);
  }
}

template<u64 base, UnsignedConcept T>
constexpr auto
round_down(T x) noexcept -> T
{
  static_assert(base > 0);
  if constexpr (base == 1) {
    return x;
  } else if constexpr (std::popcount(base) == 1) {
    constexpr auto mask = ~(base - 1);
    return x & mask;
  } else {
    T const r = x % static_cast<T>(base);
    return x - r;
  }
}

template<u64 base, UnsignedConcept T>
constexpr auto
round_up(T x) noexcept -> T
{
  static_assert(base > 0);
  if (not std::is_constant_evaluated()) {
    DEBUG_ASSERT_LE(x, std::numeric_limits<T>::max() - base + 1);
  }
  if constexpr (base == 1) {
    return x;
  } else if constexpr (std::popcount(base) == 1) {
    constexpr auto lo_mask = base - 1;
    constexpr auto hi_mask = ~lo_mask;
    return x & lo_mask ? (x & hi_mask) + base : x & hi_mask;
  } else {
    auto const r = remainder<base>(x);
    return r ? x + (static_cast<T>(base) - r) : x;
  }
}

template<u64 lo, u64 hi, UnsignedConcept T>
constexpr T
clip(T x) noexcept
{
  static_assert(lo <= hi);
  return x < lo ? static_cast<T>(lo) : (x > hi ? static_cast<T>(hi) : x);
}
