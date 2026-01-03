#pragma once

#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/bits_mixin.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/math.hpp>

#include <cstdlib>
#include <cstring>
#include <utility>

namespace kaspan {

class bits final
  : public buffer
  , public bits_mixin<bits>
{
public:
  bits() noexcept = default;
  ~bits()         = default;

  explicit bits(u64 size) noexcept(false)
    : buffer(((size + 63) / 64) * sizeof(u64))
  {
  }

  bits(bits const&) = delete;
  bits(bits&& rhs) noexcept
    : buffer(std::move(rhs))
  {
  }

  auto operator=(bits const&) -> bits& = delete;
  auto operator=(bits&& rhs) noexcept -> bits&
  {
    buffer::operator=(std::move(rhs));
    return *this;
  }

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(buffer::data());
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(buffer::data());
  }

  [[nodiscard]] explicit operator bits_accessor() noexcept
  {
    return bits_accessor{ data() };
  }

  [[nodiscard]] explicit operator bits_accessor() const noexcept
  {
    return bits_accessor{ const_cast<u64*>(data()) };
  }
};

inline auto
make_bits(u64 size) noexcept -> bits
{
  return bits{ size };
}

inline auto
make_bits_clean(u64 size) noexcept -> bits
{
  auto res       = bits{ size };
  auto byte_size = ceildiv<64>(size) * sizeof(u64);
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(res.data(), byte_size);
  std::memset(res.data(), 0x00, byte_size);
#ifdef KASPAN_DEBUG
  for (u64 i = 0; i < size; ++i) {
    ASSERT_EQ(res.get(i), false);
  }
#endif
  return res;
}

inline auto
make_bits_filled(u64 size) noexcept -> bits
{
  auto res       = bits{ size };
  auto byte_size = ceildiv<64>(size) * sizeof(u64);
  KASPAN_VALGRIND_MAKE_MEM_DEFINED(res.data(), byte_size);
  std::memset(res.data(), 0xff, byte_size);
#ifdef KASPAN_DEBUG
  for (u64 i = 0; i < size; ++i) {
    ASSERT_EQ(res.get(i), true);
  }
#endif
  return res;
}

} // namespace kaspan
