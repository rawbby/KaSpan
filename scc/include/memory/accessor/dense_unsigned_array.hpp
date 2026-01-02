#pragma once

#include <memory/accessor/dense_unsigned_accessor_mixin.hpp>
#include <memory/buffer.hpp>
#include <util/arithmetic.hpp>

#include <bit>
#include <cstddef>
#include <utility>

template<UnsignedConcept T = u64>
class DenseUnsignedArray final : public Buffer
  , public DenseUnsignedAccessorMixin<DenseUnsignedArray<T>, T>
{
public:
  DenseUnsignedArray() noexcept = default;
  ~DenseUnsignedArray()         = default;

  explicit DenseUnsignedArray(u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept(false)
    : Buffer(count * element_byte_size)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
  }

  DenseUnsignedArray(DenseUnsignedArray const&) = delete;
  DenseUnsignedArray(DenseUnsignedArray&& rhs) noexcept
    : Buffer(std::move(rhs))
    , element_byte_size_(rhs.element_byte_size_)
    , endian_(rhs.endian_)
  {
  }

  auto operator=(DenseUnsignedArray const&) -> DenseUnsignedArray& = delete;
  auto operator=(DenseUnsignedArray&& rhs) noexcept -> DenseUnsignedArray&
  {
    Buffer::operator=(std::move(rhs));
    element_byte_size_ = rhs.element_byte_size_;
    endian_            = rhs.endian_;
    return *this;
  }

  static auto create(u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept(false) -> DenseUnsignedArray
  {
    return DenseUnsignedArray{ count, element_byte_size, endian };
  }

  [[nodiscard]] auto data() -> std::byte*
  {
    return static_cast<std::byte*>(Buffer::data());
  }

  [[nodiscard]] auto data() const -> std::byte const*
  {
    return static_cast<std::byte const*>(Buffer::data());
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
