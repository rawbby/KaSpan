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

class Bits final : public Buffer
  , public BitsMixin<Bits>
{
public:
  Bits() noexcept = default;
  ~Bits()         = default;

  explicit Bits(u64 size) noexcept(false)
    : Buffer(((size + 63) / 64) * sizeof(u64))
  {
  }

  Bits(Bits const&) = delete;
  Bits(Bits&& rhs) noexcept
    : Buffer(std::move(rhs))
  {
  }

  auto operator=(Bits const&) -> Bits& = delete;
  auto operator=(Bits&& rhs) noexcept -> Bits&
  {
    Buffer::operator=(std::move(rhs));
    return *this;
  }

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(Buffer::data());
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(Buffer::data());
  }

  [[nodiscard]] operator BitsAccessor() const noexcept
  {
    return BitsAccessor{ data_ };
  }
};

inline auto
make_bits(u64 size) noexcept -> Bits
{
  return Bits{ size };
}

inline auto
make_bits_clean(u64 size) noexcept -> Bits
{
  auto&& bits      = Bits{ size };
  auto   byte_size = ceildiv<64>(size) * sizeof(u64);
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(bits.data(), byte_size);
  std::memset(bits.data(), 0x00, byte_size);
#ifdef KASPAN_DEBUG
  for (u64 i = 0; i < size; ++i)
    ASSERT_EQ(bits.get(i), false);
#endif
  return bits;
}

inline auto
make_bits_filled(u64 size) noexcept -> Bits
{
  auto&& bits      = Bits{ size };
  auto   byte_size = ceildiv<64>(size) * sizeof(u64);
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(bits.data(), byte_size);
  std::memset(bits.data(), 0xff, byte_size);
#ifdef KASPAN_DEBUG
  for (u64 i = 0; i < size; ++i)
    ASSERT_EQ(bits.get(i), true);
#endif
  return bits;
}
