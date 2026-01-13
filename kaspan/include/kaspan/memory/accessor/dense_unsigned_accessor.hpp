#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/accessor/dense_unsigned_ops.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <bit>
#include <cstddef>

namespace kaspan {

template<unsigned_concept T = u64>
class dense_unsigned_accessor final
{
public:
  template<arithmetic_concept Size>
  explicit dense_unsigned_accessor(
    void*       data,
    Size        size,
    u8          element_byte_size,
    std::endian endian = std::endian::native)
    : data_(size == 0 ? nullptr : data)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    IF(KASPAN_DEBUG, size_ = integral_cast<u64>(size));
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
    KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(data, size * element_byte_size);
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

  template<arithmetic_concept Count>
  void fill(
    T     value,
    Count n)
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_LE(n, std::numeric_limits<u64>::max());
    auto const n64 = integral_cast<u64>(n);
    DEBUG_ASSERT_LE(n64, size_);
    dense_unsigned_ops::fill<T>(data(), n64, element_bytes(), endian(), value);
  }

  template<arithmetic_concept Index>
  [[nodiscard]] auto get(
    Index index) const -> T
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    return dense_unsigned_ops::get<T>(data(), index64, element_bytes(), endian());
  }

  template<arithmetic_concept Index>
  void set(
    Index index,
    T     val)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    dense_unsigned_ops::set<T>(data(), index64, element_bytes(), endian(), val);
  }

private:
  void*       data_;
  u8          element_byte_size_;
  std::endian endian_;
  IF(
    KASPAN_DEBUG,
    u64 size_ = 0;)
};

template<unsigned_concept   T = u64,
         arithmetic_concept Count>
auto
borrow_dense_unsigned(
  void**      memory,
  Count       count,
  u8          element_byte_size,
  std::endian endian = std::endian::native) -> dense_unsigned_accessor<T>
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  void*      data    = borrow_array<byte>(memory, count64 * element_byte_size);
  return dense_unsigned_accessor<T>{ data, count64, element_byte_size, endian };
}

template<unsigned_concept   T = u64,
         arithmetic_concept Count>
auto
view_dense_unsigned(
  void*       data,
  Count       count,
  u8          element_byte_size,
  std::endian endian = std::endian::native) -> dense_unsigned_accessor<T>
{
  return dense_unsigned_accessor<T>{ data, count, element_byte_size, endian };
}

} // namespace kaspan
