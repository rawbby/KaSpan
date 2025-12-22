#pragma once

#include <memory/accessor/bits_mixin.hpp>
#include <memory/borrow.hpp>
#include <memory/buffer.hpp>
#include <util/arithmetic.hpp>

#include <bit>
#include <cstdlib>
#include <cstring>
#include <utility>

class Bits final : public Buffer, public BitsMixin<Bits>
{
public:
  Bits() noexcept = default;
  ~Bits()         = default;

  explicit Bits(u64 count) noexcept(false)
    : Buffer((count + static_cast<u64>(63)) >> 6)
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

  static auto create(u64 count) noexcept(false) -> Bits
  {
    return Bits{ count };
  }

  static auto create_clean(u64 count) noexcept(false) -> Bits
  {
    auto vec = Bits{ count };
    vec.clear(count);
    return vec;
  }

  static auto create_filled(u64 count) noexcept(false) -> Bits
  {
    auto vec = Bits{ count };
    vec.fill(count);
    return vec;
  }

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(Buffer::data());
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(Buffer::data());
  }
};
