#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>
#include <limits>

namespace kaspan {

struct bits_ops
{
  static void clear(u64* data, u64 count)
  {
    auto const idx = floordiv<64>(count);
    std::memset(data, 0, idx * sizeof(u64));
    if (auto const rem = remainder<64>(count)) {
      auto const msk = static_cast<u64>(1) << rem;
      data[idx] &= ~(msk - 1);
    }
  }

  static void fill(u64* data, u64 count)
  {
    auto const idx = floordiv<64>(count);
    std::memset(data, ~0, idx * sizeof(u64));
    if (auto const rem = remainder<64>(count)) {
      auto const msk = static_cast<u64>(1) << rem;
      data[idx] |= msk - 1;
    }
  }

  [[nodiscard]] static auto get(u64 const* data, u64 bit_index) -> bool
  {
    DEBUG_ASSERT_NE(data, nullptr);
    auto const idx = floordiv<64>(bit_index);
    auto const rem = remainder<64>(bit_index);
    auto const msk = static_cast<u64>(1) << rem;
    return (data[idx] & msk) != 0;
  }

  static void set(u64* data, u64 bit_index, bool bit_value)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    auto const val_msk = -static_cast<u64>(bit_value);
    auto const idx     = floordiv<64>(bit_index);
    auto const rem     = remainder<64>(bit_index);
    auto const msk     = static_cast<u64>(1) << rem;

    auto field = data[idx];
    field &= ~msk;
    field |= val_msk & msk;
    data[idx] = field;
  }

  static void set(u64* data, u64 bit_index)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    auto const idx = floordiv<64>(bit_index);
    auto const rem = remainder<64>(bit_index);
    auto const msk = static_cast<u64>(1) << rem;
    data[idx] |= msk;
  }

  static void unset(u64* data, u64 bit_index)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    auto const idx = floordiv<64>(bit_index);
    auto const rem = remainder<64>(bit_index);
    auto const msk = static_cast<u64>(1) << rem;
    data[idx] &= ~msk;
  }

  template<arithmetic_concept Index = size_t, std::invocable<Index> fn_t>
  static void for_each(u64 const* data, Index end, fn_t&& fn)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end, 0, std::numeric_limits<u64>::max());

    auto const c64 = floordiv<64>(static_cast<u64>(end));
    auto const rem = remainder<64>(static_cast<u64>(end));

    for (u64 i = 0; i < c64; ++i) {
      u64 word = data[i];
      while (word) {
        fn(static_cast<Index>((i << 6) + std::countr_zero(word)));
        word &= word - 1;
      }
    }
    if (rem) {
      auto word = data[c64] & ((static_cast<u64>(1) << rem) - 1);
      while (word) {
        fn(static_cast<Index>((c64 << 6) + std::countr_zero(word)));
        word &= word - 1;
      }
    }
  }

  template<arithmetic_concept Index = size_t, std::invocable<Index> fn_t>
  static void set_each(u64* data, Index end, fn_t&& fn)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_NE(data, nullptr);

    auto const end64 = static_cast<u64>(end);
    auto const c64 = floordiv<64>(end64);
    u64        b64 = 0;

    for (u64 i = 0; i < c64; ++i, b64 += 64) {
      u64 w = 0;
      for (unsigned bit = 0; bit < 64; ++bit) {
        w |= static_cast<u64>(!!fn(b64 + bit)) << bit;
      }
      data[i] = w;
    }

    if (auto const rem = remainder<64>(end64)) {
      u64 w = 0;
      for (unsigned bit = 0; bit < rem; ++bit) {
        w |= static_cast<u64>(!!fn(b64 + bit)) << bit;
      }
      auto const mask = (static_cast<u64>(1) << rem) - 1;
      data[c64]       = (data[c64] & ~mask) | (w & mask);
    }
  }
};

} // namespace kaspan
