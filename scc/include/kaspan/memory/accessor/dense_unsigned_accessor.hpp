#pragma once

#include <kaspan/memory/accessor/dense_unsigned_mixin.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <cstddef>
#include <cstdlib>

namespace kaspan {

template<unsigned_concept T = u64>
class dense_unsigned_accessor final : public dense_unsigned_mixin<dense_unsigned_accessor<T>, T>
{
public:
  explicit dense_unsigned_accessor(void* data, u8 element_byte_size, std::endian endian = std::endian::native)
    : data_(data)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    DEBUG_ASSERT(is_page_aligned(data));
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
  }

  dense_unsigned_accessor()  = delete;
  ~dense_unsigned_accessor() = default;

  dense_unsigned_accessor(dense_unsigned_accessor const& rhs) noexcept = default;
  dense_unsigned_accessor(dense_unsigned_accessor&& rhs) noexcept      = default;

  auto operator=(dense_unsigned_accessor const& rhs) noexcept -> dense_unsigned_accessor& = default;
  auto operator=(dense_unsigned_accessor&& rhs) noexcept -> dense_unsigned_accessor&      = default;

  static auto borrow(void** memory, u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept -> dense_unsigned_accessor
  {
    return dense_unsigned_accessor{ borrow_array(memory, count * element_byte_size), element_byte_size, endian };
  }

  static auto view(void* memory, u8 element_byte_size, std::endian endian = std::endian::native) noexcept -> dense_unsigned_accessor
  {
    return dense_unsigned_accessor{ memory, element_byte_size, endian };
  }

  auto data() -> std::byte*
  {
    return static_cast<std::byte*>(data_);
  }

  [[nodiscard]] auto data() const -> std::byte const*
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

} // namespace kaspan
