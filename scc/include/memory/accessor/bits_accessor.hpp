#pragma once

#include <memory/accessor/bits_mixin.hpp>
#include <memory/borrow.hpp>
#include <util/arithmetic.hpp>
#include <util/math.hpp>

class bits_accessor final : public bits_mixin<bits_accessor>
{
public:
  explicit bits_accessor(void* data)
    : data_(data)
  {
  }

  bits_accessor()  = default;
  ~bits_accessor() = default;

  bits_accessor(bits_accessor const& rhs) noexcept = default;
  bits_accessor(bits_accessor&& rhs) noexcept      = default;

  auto operator=(bits_accessor const& rhs) noexcept -> bits_accessor& = default;
  auto operator=(bits_accessor&& rhs) noexcept -> bits_accessor&      = default;

  [[nodiscard]] auto data() -> u64* { return static_cast<u64*>(data_); }

  [[nodiscard]] auto data() const -> u64 const* { return static_cast<u64 const*>(data_); }

private:
  void* data_ = nullptr;
};

inline auto
borrow_bits(void** memory, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ borrow_array<u64>(memory, ceildiv<64>(size)) };
}

inline auto
borrow_bits_clean(void** memory, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ borrow_array_clean<u64>(memory, ceildiv<64>(size)) };
}

inline auto
borrow_bits_filled(void** memory, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ borrow_array_filled<u64>(memory, ceildiv<64>(size)) };
}
