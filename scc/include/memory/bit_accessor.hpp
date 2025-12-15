#pragma once

#include <util/arithmetic.hpp>

#include <bit>
#include <cstdlib>
#include <cstring>
#include <utility>

class BitAccessor final
{
public:
  /// this bit accessor is assumed to be created from a buffer that is os page aligned
  /// meaning it is always suitable to be used as u64 array. further this accessor is non owning!
  /// notice: this class explicitly does not track size! This is a low level accessor that is ment to be used within a safe context!
  explicit BitAccessor(void* data)
    : data_(data)
  {
  }

  BitAccessor()  = default;
  ~BitAccessor() = default;

  BitAccessor(BitAccessor const& rhs) noexcept = default;
  BitAccessor(BitAccessor&& rhs) noexcept      = default;

  auto operator=(BitAccessor const& rhs) noexcept -> BitAccessor& = default;
  auto operator=(BitAccessor&& rhs) noexcept -> BitAccessor&      = default;

  static auto borrow(void*& memory, u64 count) noexcept -> BitAccessor
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    return BitAccessor{ ::borrow<u64>(memory, u64_count) };
  }

  static auto borrow_clean(void*& memory, u64 count) noexcept -> BitAccessor
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    return BitAccessor{ ::borrow_clean<u64>(memory, u64_count) };
  }

  static auto borrow_filled(void*& memory, u64 count) noexcept -> BitAccessor
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    return BitAccessor{ ::borrow_filled<u64>(memory, u64_count) };
  }

  static auto create(u64 count) noexcept -> std::pair<Buffer, BitAccessor>
  {
    auto  buffer = Buffer(((count + static_cast<u64>(63)) >> 6) * sizeof(u64));
    auto* memory = buffer.data();
    return { std::move(buffer), BitAccessor{ memory } };
  }

  static auto create_clean(u64 count) noexcept -> std::pair<Buffer, BitAccessor>
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    auto       buffer    = Buffer{ u64_count * sizeof(u64) };
    auto*      memory    = buffer.data();
    return { std::move(buffer), BitAccessor{ ::borrow_clean<u64>(memory, u64_count) } };
  }

  static auto create_filled(u64 count) noexcept -> std::pair<Buffer, BitAccessor>
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    auto       buffer    = Buffer{ u64_count * sizeof(u64) };
    auto*      memory    = buffer.data();
    return { std::move(buffer), BitAccessor{ ::borrow_filled<u64>(memory, ~0, u64_count) } };
  }

  void clear(u64 count)
  {
    auto const idx = count >> 6;
    std::memset(data_, 0, idx * sizeof(u64));
    if (auto const rem = count & static_cast<u64>(63)) {
      auto const msk = static_cast<u64>(1) << rem;
      data()[idx] &= ~(msk - 1);
    }
  }

  void fill(u64 count)
  {
    auto const idx = count >> 6;
    std::memset(data_, ~0, idx * sizeof(u64));
    if (auto const rem = count & static_cast<u64>(63)) {
      auto const msk = static_cast<u64>(1) << rem;
      data()[idx] |= msk - 1;
    }
  }

  [[nodiscard]] auto get(u64 bit_index) -> bool
  {
    DEBUG_ASSERT_NE(data(), nullptr);
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    return (data()[idx] & msk) != 0;
  }

  void set(u64 bit_index, bool bit_value)
  {
    DEBUG_ASSERT_NE(data(), nullptr);
    auto const val_msk = -static_cast<u64>(bit_value);
    auto const idx     = bit_index >> 6;
    auto const rem     = bit_index & static_cast<u64>(63);
    auto const msk     = static_cast<u64>(1) << rem;

    auto field = data()[idx]; // single load
    field &= ~msk;            // unconditional unset
    field |= val_msk & msk;   // conditional set
    data()[idx] = field;      // single store
  }

  void set(u64 bit_index)
  {
    DEBUG_ASSERT_NE(data(), nullptr);
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    data()[idx] |= msk;
  }

  void
  unset(u64 bit_index)
  {
    DEBUG_ASSERT_NE(data(), nullptr);
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    data()[idx] &= ~msk;
  }

  template<ArithmeticConcept Index = size_t, std::invocable<Index> Fn>
  void for_each(Index count, Fn&& fn)
  {
    DEBUG_ASSERT(data() != nullptr or count == 0);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(count, 0, std::numeric_limits<u64>::max());

    auto const c64 = static_cast<u64>(count) >> 6;
    auto const rem = static_cast<u64>(count) & static_cast<u64>(63);

    for (u64 i = 0; i < c64; ++i) {
      u64 word = data()[i];
      while (word) {
        fn(static_cast<Index>((i << 6) + std::countr_zero(word)));
        word &= word - 1;
      }
    }
    if (rem) {
      auto word = data()[c64] & (static_cast<u64>(1) << rem) - 1;
      while (word) {
        fn(static_cast<Index>((c64 << 6) + std::countr_zero(word)));
        word &= word - 1;
      }
    }
  }

  template<typename Compare = std::equal_to<>, typename T>
  void fill_cmp(u64 count, T const* tx, T const& t, Compare&& cmp = Compare{})
  {
    DEBUG_ASSERT(data() != nullptr or count == 0);

    auto const c64 = count >> 6;
    u64        b64 = 0;

    for (u64 i = 0; i < c64; ++i, b64 += 64) {
      u64 w = 0;
      for (unsigned bit = 0; bit < 64; ++bit) {
        w |= static_cast<u64>(!!cmp(tx[b64 + bit], t)) << bit;
      }
      data()[i] = w;
    }

    if (auto const rem = count & static_cast<u64>(63)) {
      u64 w = 0;
      for (unsigned bit = 0; bit < rem; ++bit) {
        w |= static_cast<u64>(!!cmp(tx[b64 + bit], t)) << bit;
      }
      auto const mask = (static_cast<u64>(1) << rem) - 1;
      data()[c64]     = data()[c64] & ~mask | w & mask;
    }
  }

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(data_);
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(data_);
  }

private:
  void* data_ = nullptr;
};
