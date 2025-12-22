#pragma once

#include <memory/accessor/bits_mixin.hpp>
#include <util/arithmetic.hpp>
#include <util/math.hpp>

class BitsAccessor final : public BitsMixin<BitsAccessor>
{
public:
  explicit BitsAccessor(void* data)
    : data_(data)
  {
  }

  BitsAccessor()  = default;
  ~BitsAccessor() = default;

  BitsAccessor(BitsAccessor const& rhs) noexcept = default;
  BitsAccessor(BitsAccessor&& rhs) noexcept      = default;

  auto operator=(BitsAccessor const& rhs) noexcept -> BitsAccessor& = default;
  auto operator=(BitsAccessor&& rhs) noexcept -> BitsAccessor&      = default;

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

inline auto
borrow_bits(void** memory, u64 size) noexcept -> BitsAccessor
{
  return BitsAccessor{ borrow_array<u64>(memory, ceildiv<64>(size)) };
}

inline auto
borrow_bits_clean(void** memory, u64 size) noexcept -> BitsAccessor
{
  return BitsAccessor{ borrow_array_clean<u64>(memory, ceildiv<64>(size)) };
}

inline auto
borrow_bits_filled(void** memory, u64 size) noexcept -> BitsAccessor
{
  return BitsAccessor{ borrow_array_filled<u64>(memory, ceildiv<64>(size)) };
}
