#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <bitset>
#include <cstring>

namespace kaspan::bits_ops {

[[gnu::always_inline]] constexpr void
clear(
  u64*                    data,
  integral_c auto end) noexcept
{
  DEBUG_ASSERT_GE(end, 0);
  DEBUG_ASSERT(end == 0 || data != nullptr);
  std::fill_n(data, end >> 6, static_cast<u64>(0));
  if (auto const rem = end & 0b111111) {
    data[end >> 6] &= ~((static_cast<u64>(1) << rem) - 1);
  }
}

[[gnu::always_inline]] constexpr void
fill(
  u64*                    data,
  integral_c auto end) noexcept
{
  DEBUG_ASSERT_GE(end, 0);
  DEBUG_ASSERT(end == 0 || data != nullptr);
  std::fill_n(data, end >> 6, ~static_cast<u64>(0));
  if (auto const rem = end & 0b111111) {
    data[end >> 6] |= (static_cast<u64>(1) << rem) - 1;
  }
}

[[gnu::always_inline]] [[nodiscard]] constexpr auto
get(
  u64 const*              data,
  integral_c auto index) noexcept -> bool
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  return data[index >> 6] & static_cast<u64>(1) << (index & 0b111111);
}

[[gnu::always_inline]] constexpr void
set(
  u64*                    data,
  integral_c auto index,
  bool                    f) noexcept
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  auto const mask  = static_cast<u64>(1) << (index & 0b111111);
  auto&      value = data[index >> 6];
  value            = value & ~mask | -static_cast<u64>(f) & mask;
}

[[gnu::always_inline]] constexpr void
set(
  u64*                    data,
  integral_c auto index) noexcept
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  data[index >> 6] |= static_cast<u64>(1) << (index & 0b111111);
}

[[gnu::always_inline]] constexpr void
unset(
  u64*                    data,
  integral_c auto index) noexcept
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  data[index >> 6] &= ~(static_cast<u64>(1) << (index & 0b111111));
}

[[gnu::always_inline]] constexpr auto
getset(
  u64*                    data,
  integral_c auto index,
  bool                    f) noexcept -> bool
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  auto const mask   = static_cast<u64>(1) << (index & 0b111111);
  auto&      value  = data[index >> 6];
  auto const result = value & mask;
  value             = value & ~mask | -static_cast<u64>(f) & mask;
  return result;
}

[[gnu::always_inline]] constexpr auto
getset(
  u64*                    data,
  integral_c auto index) noexcept -> bool
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  auto const mask   = static_cast<u64>(1) << (index & 0b111111);
  auto&      value  = data[index >> 6];
  auto const result = value & mask;
  value |= mask;
  return result;
}

[[gnu::always_inline]] constexpr auto
getunset(
  u64*                    data,
  integral_c auto index) noexcept -> bool
{
  DEBUG_ASSERT_GE(index, 0);
  DEBUG_ASSERT_NE(data, nullptr);
  auto const mask   = static_cast<u64>(1) << (index & 0b111111);
  auto&      value  = data[index >> 6];
  auto const result = value & mask;
  value &= ~mask;
  return result;
}

[[gnu::always_inline]] inline void
each(
  u64 const*              data,
  integral_c auto end,
  auto&&                  fn)
{
  DEBUG_ASSERT_GE(end, 0);
  DEBUG_ASSERT(end == 0 || data != nullptr);

  auto const end_floor = ~static_cast<decltype(end)>(0b111111) & end;
  for (decltype(end) i = 0; i < end_floor; i += 64) {
    u64 word = data[i >> 6];
    while (word) {
      fn(i + std::countr_zero(word));
      word &= word - 1;
    }
  }

  if (auto const rem = end & 0b111111) {
    auto word = data[end >> 6] & (static_cast<u64>(1) << rem) - 1;
    while (word) {
      fn(end_floor + std::countr_zero(word));
      word &= word - 1;
    }
  }
}

[[gnu::always_inline]] inline void
set_each(
  u64*                    data,
  integral_c auto end,
  auto&&                  fn)
{
  DEBUG_ASSERT_GE(end, 0);
  DEBUG_ASSERT(end == 0 || data != nullptr);

  auto const end_floor = ~static_cast<decltype(end)>(0b111111) & end;
  for (decltype(end) i = 0; i < end_floor; i += 64) {
    u64 word = 0;
    for (u8 bit = 0; bit < 64; ++bit) {
      word |= static_cast<u64>(!!fn(i + bit)) << bit;
    }
    data[i >> 6] = word;
  }

  if (auto const rem = end & 0b111111) {
    u64 word = 0;
    for (u8 bit = 0; bit < rem; ++bit) {
      word |= static_cast<u64>(!!fn(end_floor + bit)) << bit;
    }
    auto const idx = end >> 6;
    auto const msk = (static_cast<u64>(1) << rem) - 1;
    data[idx]      = data[idx] & ~msk | word & msk;
  }
}

} // namespace kaspan::bits_ops
