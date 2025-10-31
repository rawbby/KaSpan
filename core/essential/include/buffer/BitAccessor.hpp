#pragma once

#include "debug/Assert.hpp"

#include <util/Arithmetic.hpp>
#include <util/Util.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <type_traits>

class BitAccessor final
{
public:
  /// this bit accessor is assumed to be created from a buffer that is os page aligned
  /// meaning it is always suitable to be used as u64 array. further this accessor is non owning!
  /// notice: this class explicitly does not track size! This is a low level accessor that is ment to be used within a safe context!
  explicit BitAccessor(void* __restrict data)
    : data_(data)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    DEBUG_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(data) & (alignof(u64) - 1), 0);
  }

  BitAccessor()  = delete;
  ~BitAccessor() = default;

  BitAccessor(BitAccessor const& rhs) noexcept = default;
  BitAccessor(BitAccessor&& rhs) noexcept      = default;

  auto operator=(BitAccessor const& rhs) noexcept -> BitAccessor& = default;
  auto operator=(BitAccessor&& rhs) noexcept -> BitAccessor&      = default;

  static auto borrow(void* __restrict& memory, u64 n) noexcept -> BitAccessor
  {
    DEBUG_ASSERT(page_aligned(memory));
    auto const result    = BitAccessor{ memory };
    auto const u64_count = (n + static_cast<u64>(63)) >> 6;
    auto const byte_size = page_ceil(u64_count * sizeof(u64));
    memory               = static_cast<void*>(static_cast<std::byte*>(memory) + byte_size);
    return result;
  }

  static auto borrow_clean(void* __restrict& memory, u64 n) noexcept -> BitAccessor
  {
    auto result = borrow(memory, n);
    result.clear(n);
    return result;
  }

  static auto borrow_filled(void* __restrict& memory, u64 n) noexcept -> BitAccessor
  {
    auto result = borrow(memory, n);
    result.fill(n);
    return result;
  }

  void clear(u64 bits)
  {
    auto const idx = bits >> 6;
    std::memset(data_, 0, idx * sizeof(u64));
    if (auto const rem = bits & static_cast<u64>(63)) {
      auto const msk = static_cast<u64>(1) << rem;
      data()[idx] &= ~(msk - 1);
    }
  }

  void fill(u64 bits)
  {
    auto const idx = bits >> 6;
    std::memset(data_, ~0, idx * sizeof(u64));
    if (auto const rem = bits & static_cast<u64>(63)) {
      auto const msk = static_cast<u64>(1) << rem;
      data()[idx] |= msk - 1;
    }
  }

  [[nodiscard]] auto get(u64 bit_index) -> bool
  {
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    return (data()[idx] & msk) != 0;
  }

  void set(u64 bit_index, bool value)
  {
    auto const val_msk = -static_cast<u64>(value);
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
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    data()[idx] |= msk;
  }

  void
  unset(u64 bit_index)
  {
    auto const idx = bit_index >> 6;
    auto const rem = bit_index & static_cast<u64>(63);
    auto const msk = static_cast<u64>(1) << rem;
    data()[idx] &= ~msk;
  }

  u64* data()
  {
    return static_cast<u64*>(data_);
  }

private:
  void* data_;
};
