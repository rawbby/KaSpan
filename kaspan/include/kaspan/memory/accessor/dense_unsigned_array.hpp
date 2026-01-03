#pragma once

#include <kaspan/memory/accessor/dense_unsigned_mixin.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <cstddef>
#include <utility>

namespace kaspan {

template<unsigned_concept T = u64>
class dense_unsigned_array final
  : public buffer
  , public dense_unsigned_mixin<dense_unsigned_array<T>, T>
{
public:
  dense_unsigned_array() noexcept = default;
  ~dense_unsigned_array()         = default;

  explicit dense_unsigned_array(u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept(false)
    : buffer(count * element_byte_size)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
  }

  dense_unsigned_array(dense_unsigned_array const&) = delete;
  dense_unsigned_array(dense_unsigned_array&& rhs) noexcept
    : buffer(std::move(rhs))
    , element_byte_size_(rhs.element_byte_size_)
    , endian_(rhs.endian_)
  {
  }

  auto operator=(dense_unsigned_array const&) -> dense_unsigned_array& = delete;
  auto operator=(dense_unsigned_array&& rhs) noexcept -> dense_unsigned_array&
  {
    buffer::operator=(std::move(rhs));
    element_byte_size_ = rhs.element_byte_size_;
    endian_            = rhs.endian_;
    return *this;
  }

  static auto create(u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept(false) -> dense_unsigned_array
  {
    return dense_unsigned_array{ count, element_byte_size, endian };
  }

  [[nodiscard]] auto data() -> std::byte*
  {
    return static_cast<std::byte*>(buffer::data());
  }

  [[nodiscard]] auto data() const -> std::byte const*
  {
    return static_cast<std::byte const*>(buffer::data());
  }

  [[nodiscard]] auto element_bytes() const -> u8
  {
    return element_byte_size_;
  }

  [[nodiscard]] auto endian() const -> std::endian
  {
    return endian_;
  }

private:
  u8          element_byte_size_ = 0;
  std::endian endian_            = std::endian::native;
};

} // namespace kaspan
