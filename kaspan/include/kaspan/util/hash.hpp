#pragma once

#include <kaspan/util/arithmetic.hpp>

namespace kaspan {

/// fmix32 (high performance, non cryptograpic, good quality / distribution ration)
[[gnu::const]] constexpr inline auto
hash(
  u32_concept auto x) noexcept -> u32
{
  x = (x ^ x >> 16) * 0x85ebca6b;
  x = (x ^ x >> 13) * 0xc2b2ae35;
  return x ^ x >> 16;
}

/// fmix64 (high performance, non cryptograpic, good quality / distribution ration)
[[gnu::const]] constexpr inline auto
hash(
  u64_concept auto x) noexcept -> u64
{
  x = (x ^ x >> 30) * 0xbf58476d1ce4e5b9;
  x = (x ^ x >> 27) * 0x94d049bb133111eb;
  return x ^ x >> 31;
}

}
