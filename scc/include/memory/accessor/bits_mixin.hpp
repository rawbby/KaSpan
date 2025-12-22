#pragma once

#include <util/arithmetic.hpp>

#include <bit>
#include <cstring>
#include <functional>
#include <limits>

template<typename Derived>
class BitsMixin
{
public:
  void clear(u64 count)
  {
    auto const idx = count >> 6;
    std::memset(derived()->data(), 0, idx * sizeof(u64));
    if (auto const rem = count & static_cast<u64>(63)) {
      auto const msk = static_cast<u64>(1) << rem;
      derived()->data()[idx] &= ~(msk - 1);
    }
  }

  void fill(u64 count)
  {
    auto const idx = count >> 6;
    std::memset(derived()->data(), ~0, idx * sizeof(u64));
    if (auto const rem = count & static_cast<u64>(63)) {
      auto const msk = static_cast<u64>(1) << rem;
      derived()->data()[idx] |= msk - 1;
    }
  }

  [[nodiscard]] auto get(u64 bit_index) -> bool
  {
    DEBUG_ASSERT_NE(derived()->data(), nullptr);
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    return (derived()->data()[idx] & msk) != 0;
  }

  void set(u64 bit_index, bool bit_value)
  {
    DEBUG_ASSERT_NE(derived()->data(), nullptr);
    auto const val_msk = -static_cast<u64>(bit_value);
    auto const idx     = bit_index >> 6;
    auto const rem     = bit_index & static_cast<u64>(63);
    auto const msk     = static_cast<u64>(1) << rem;

    auto field = derived()->data()[idx]; // single load
    field &= ~msk;                        // unconditional unset
    field |= val_msk & msk;               // conditional set
    derived()->data()[idx] = field;       // single store
  }

  void set(u64 bit_index)
  {
    DEBUG_ASSERT_NE(derived()->data(), nullptr);
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    derived()->data()[idx] |= msk;
  }

  void unset(u64 bit_index)
  {
    DEBUG_ASSERT_NE(derived()->data(), nullptr);
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    derived()->data()[idx] &= ~msk;
  }

  template<ArithmeticConcept Index = size_t, std::invocable<Index> Fn>
  void for_each(Index count, Fn&& fn)
  {
    DEBUG_ASSERT(derived()->data() != nullptr or count == 0);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(count, 0, std::numeric_limits<u64>::max());

    auto const c64 = static_cast<u64>(count) >> 6;
    auto const rem = static_cast<u64>(count) & static_cast<u64>(63);

    for (u64 i = 0; i < c64; ++i) {
      u64 word = derived()->data()[i];
      while (word) {
        fn(static_cast<Index>((i << 6) + std::countr_zero(word)));
        word &= word - 1;
      }
    }
    if (rem) {
      auto word = derived()->data()[c64] & (static_cast<u64>(1) << rem) - 1;
      while (word) {
        fn(static_cast<Index>((c64 << 6) + std::countr_zero(word)));
        word &= word - 1;
      }
    }
  }

  template<typename Compare = std::equal_to<>, typename T>
  void fill_cmp(u64 count, T const* tx, T const& t, Compare&& cmp = Compare{})
  {
    DEBUG_ASSERT(derived()->data() != nullptr or count == 0);

    auto const c64 = count >> 6;
    u64        b64 = 0;

    for (u64 i = 0; i < c64; ++i, b64 += 64) {
      u64 w = 0;
      for (unsigned bit = 0; bit < 64; ++bit) {
        w |= static_cast<u64>(!!cmp(tx[b64 + bit], t)) << bit;
      }
      derived()->data()[i] = w;
    }

    if (auto const rem = count & static_cast<u64>(63)) {
      u64 w = 0;
      for (unsigned bit = 0; bit < rem; ++bit) {
        w |= static_cast<u64>(!!cmp(tx[b64 + bit], t)) << bit;
      }
      auto const mask = (static_cast<u64>(1) << rem) - 1;
      derived()->data()[c64]     = derived()->data()[c64] & ~mask | w & mask;
    }
  }

protected:
  ~BitsMixin() = default;

private:
  auto derived() -> Derived*
  {
    return static_cast<Derived*>(this);
  }

  auto derived() const -> Derived const*
  {
    return static_cast<Derived const*>(this);
  }
};
