#pragma once

#include <kaspan/memory/accessor/dense_unsigned_ops.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <cstddef>

namespace kaspan {

template<unsigned_concept T = u64>
class dense_unsigned_accessor final
{
public:
  explicit dense_unsigned_accessor(void* data, u64 size, u8 element_byte_size, std::endian endian = std::endian::native)
    : data_(size == 0 ? nullptr : data)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    IF(KASPAN_DEBUG, size_ = size);
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
  }

  dense_unsigned_accessor()  = delete;
  ~dense_unsigned_accessor() = default;

  dense_unsigned_accessor(dense_unsigned_accessor const& rhs) noexcept = default;
  dense_unsigned_accessor(dense_unsigned_accessor&& rhs) noexcept      = default;

  auto operator=(dense_unsigned_accessor const& rhs) noexcept -> dense_unsigned_accessor& = default;
  auto operator=(dense_unsigned_accessor&& rhs) noexcept -> dense_unsigned_accessor&      = default;

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

  void fill(T value, u64 n)
  {
    DEBUG_ASSERT_LE(n, size_);
    dense_unsigned_ops::fill<T>(data(), n, element_bytes(), endian(), value);
  }

  [[nodiscard]] auto get(u64 index) const -> T
  {
    DEBUG_ASSERT_LT(index, size_);
    return dense_unsigned_ops::get<T>(data(), index, element_bytes(), endian());
  }

  void set(u64 index, T val)
  {
    DEBUG_ASSERT_LT(index, size_);
    dense_unsigned_ops::set<T>(data(), index, element_bytes(), endian(), val);
  }

private:
  void*       data_;
  u8          element_byte_size_;
  std::endian endian_;
  IF(KASPAN_DEBUG, u64 size_ = 0;)
};

template<unsigned_concept T = u64>
auto
borrow_dense_unsigned(void** memory, u64 count, u8 element_byte_size, std::endian endian = std::endian::native) -> dense_unsigned_accessor<T>
{
  void* data = borrow_array<byte>(memory, count * element_byte_size);
  return dense_unsigned_accessor<T>{ data, count, element_byte_size, endian };
}

template<unsigned_concept T = u64>
auto
view_dense_unsigned(void* data, u64 count, u8 element_byte_size, std::endian endian = std::endian::native) -> dense_unsigned_accessor<T>
{
  return dense_unsigned_accessor<T>{ data, count, element_byte_size, endian };
}

} // namespace kaspan
