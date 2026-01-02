#pragma once

#include <memory/accessor/dense_unsigned_mixin.hpp>
#include <memory/borrow.hpp>
#include <util/arithmetic.hpp>

#include <bit>
#include <cstddef>
#include <cstdlib>

template<UnsignedConcept T = u64>
class DenseUnsignedAccessor final : public DenseUnsignedMixin<DenseUnsignedAccessor<T>, T>
{
public:
  explicit DenseUnsignedAccessor(void* data, u8 element_byte_size, std::endian endian = std::endian::native)
    : data_(data)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    DEBUG_ASSERT(is_page_aligned(data));
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
  }

  DenseUnsignedAccessor()  = delete;
  ~DenseUnsignedAccessor() = default;

  DenseUnsignedAccessor(DenseUnsignedAccessor const& rhs) noexcept = default;
  DenseUnsignedAccessor(DenseUnsignedAccessor&& rhs) noexcept      = default;

  auto operator=(DenseUnsignedAccessor const& rhs) noexcept -> DenseUnsignedAccessor& = default;
  auto operator=(DenseUnsignedAccessor&& rhs) noexcept -> DenseUnsignedAccessor&      = default;

  static auto borrow(void** memory, u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept
    -> DenseUnsignedAccessor
  {
    return DenseUnsignedAccessor{ ::borrow_array(memory, count * element_byte_size), element_byte_size, endian };
  }

  static auto view(void* memory, u8 element_byte_size, std::endian endian = std::endian::native) noexcept
    -> DenseUnsignedAccessor
  {
    return DenseUnsignedAccessor{ memory, element_byte_size, endian };
  }

  auto data() -> std::byte*
  {
    return static_cast<std::byte*>(data_);
  }

  auto data() const -> std::byte const*
  {
    return static_cast<std::byte const*>(data_);
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
  void*       data_;
  u8          element_byte_size_;
  std::endian endian_;
};
