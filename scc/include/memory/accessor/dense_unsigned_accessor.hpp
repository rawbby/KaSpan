#pragma once

#include <memory/borrow.hpp>
#include <util/arithmetic.hpp>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <span>

template<UnsignedConcept T = u64>
class DenseUnsignedAccessor final
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

  static auto borrow(void* & memory, u64 count, u8 element_byte_size, std::endian endian = std::endian::native) noexcept
    -> DenseUnsignedAccessor
  {
    return DenseUnsignedAccessor{ ::borrow(memory, count * element_byte_size), element_byte_size, endian };
  }

  static auto view(void*  memory, u8 element_byte_size, std::endian endian = std::endian::native) noexcept
    -> DenseUnsignedAccessor
  {
    return DenseUnsignedAccessor{ memory, element_byte_size, endian };
  }

  void fill(T value, u64 n)
  {
    for (u64 i = 0; i < n; ++i) {
      set(i, value);
    }
  }

  [[nodiscard]] auto get(u64 index) const -> T
  {
    T          result{};
    auto       result_view = std::as_writable_bytes(std::span{ &result, 1 });
    auto const data_view   = std::as_bytes(std::span{ data() + index * element_byte_size_, element_byte_size_ });

    // ReSharper disable CppDFAUnreachableCode
    if (std::endian::native == std::endian::little) {
      if (endian_ == std::endian::big) {                                           // rv = 03 02 01
        std::memcpy(result_view.data(), data_view.data(), element_byte_size_);     // bv = 03 02 01 00
        std::reverse(result_view.data(), result_view.data() + element_byte_size_); // bv = 01 02 03 00
      } else {                                                                     // rv = 01 02 03
        std::memcpy(result_view.data(), data_view.data(), element_byte_size_);     // bv = 01 02 03 00
      }
    } else {
      if (endian_ == std::endian::little) {                                    // rv = 01 02 03
        std::memcpy(result_view.data(), data_view.data(), element_byte_size_); // bv = 01 02 03 00
        std::reverse(result_view.data(), result_view.data() + sizeof(T));      // bv = 00 03 02 01
      } else {                                                                 // rv = 03 02 01
        auto const offset = sizeof(T) - element_byte_size_;
        std::memcpy(result_view.data() + offset, data_view.data(), element_byte_size_); // bv = 00 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode
    return result;
  }

  void set(u64 index, T val)
  {
    auto value_view = std::as_writable_bytes(std::span{ &val, 1 });
    auto data_view  = std::as_writable_bytes(std::span{ data() + index * element_byte_size_, element_byte_size_ });

    // ReSharper disable CppDFAUnreachableCode
    if (std::endian::native == std::endian::little) { // bv = 01 02 03 00
      if (endian_ == std::endian::big) {
        std::memcpy(data_view.data(), value_view.data(), element_byte_size_);  // rv = 01 02 03
        std::reverse(data_view.data(), data_view.data() + element_byte_size_); // rv = 03 02 01
      } else {
        std::memcpy(data_view.data(), value_view.data(), element_byte_size_); // rv = 01 02 03
      }
    } else { // bv = 00 03 02 01
      if (endian_ == std::endian::little) {
        std::reverse(value_view.data(), value_view.data() + sizeof(T));       // bv = 01 02 03 00
        std::memcpy(data_view.data(), value_view.data(), element_byte_size_); // rv = 01 02 03
      } else {
        auto const offset = sizeof(T) - element_byte_size_;
        std::memcpy(data_view.data(), value_view.data() + offset, element_byte_size_); // rv = 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode
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
