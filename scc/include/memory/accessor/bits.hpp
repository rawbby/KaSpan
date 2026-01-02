#pragma once

#include "scc/include/util/arithmetic.hpp"

#include <memory/accessor/bits_accessor.hpp>
#include <memory/accessor/bits_mixin.hpp>
#include <memory/borrow.hpp>
#include <memory/buffer.hpp>
#include <util/arithmetic.hpp>
#include <util/math.hpp>

#include <bit>
#include <cstdlib>
#include <cstring>
#include <utility>

class bits final
  : public Buffer
  , public bits_mixin<bits>
{
public:
  bits() noexcept = default;
  ~bits()         = default;

  explicit bits(u64 size) noexcept(false)
    : Buffer(((size + 63) / 64) * sizeof(u64))
  {
  }

  bits(bits const&) = delete;
  bits(bits&& rhs) noexcept
    : Buffer(std::move(rhs))
  {
  }

  auto operator=(bits const&) -> bits& = delete;
  auto operator=(bits&& rhs) noexcept -> bits&
  {
    Buffer::operator=(std::move(rhs));
    return *this;
  }

  [[nodiscard]] auto data() -> u64* { return static_cast<u64*>(Buffer::data()); }

  [[nodiscard]] auto data() const -> u64 const* { return static_cast<u64 const*>(Buffer::data()); }

  [[nodiscard]] explicit operator bits_accessor() const noexcept { return bits_accessor{ data_ }; }
};

inline auto
make_bits(u64 size) noexcept -> bits
{
  return bits{ size };
}

inline auto
make_bits_clean(u64 size) noexcept -> bits
{
  auto&& bits      = bits{ size };
  auto   byte_size = ceildiv<64>(size) * sizeof(u64);
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(bits.data(), byte_size);
  std::memset(bits.data(), 0x00, byte_size);
#ifdef KASPAN_DEBUG
  for (u64 i = 0; i < size; ++i) { ASSERT_EQ(bits.get(i), false);
}
#endif
  return bits;
}

inline auto
make_bits_filled(u64 size) noexcept -> bits
{
  auto&& bits      = bits{ size };
  auto   byte_size = ceildiv<64>(size) * sizeof(u64);
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(bits.data(), byte_size);
  std::memset(bits.data(), 0xff, byte_size);
#ifdef KASPAN_DEBUG
  for (u64 i = 0; i < size; ++i) { ASSERT_EQ(bits.get(i), true);
}
#endif
  return bits;
}
