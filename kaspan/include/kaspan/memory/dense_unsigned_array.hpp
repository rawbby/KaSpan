#pragma once

#include <kaspan/memory/dense_unsigned_ops.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <bit>
#include <cstddef>
#include <utility>

namespace kaspan {

template<unsigned_c T = u64>
class dense_unsigned_array final : public buffer
{
public:
  dense_unsigned_array() noexcept = default;
  ~dense_unsigned_array()         = default;

  template<integral_c Size>
  explicit dense_unsigned_array(Size size, u8 element_byte_size, std::endian endian = std::endian::native) noexcept(false)
    : buffer(integral_cast<u64>(size) * element_byte_size)
    , element_byte_size_(element_byte_size)
    , endian_(endian)
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    IF(KASPAN_DEBUG, size_ = integral_cast<u64>(size));
    DEBUG_ASSERT_GE(element_byte_size, 1);
    DEBUG_ASSERT_LE(element_byte_size, sizeof(T));
  }

  dense_unsigned_array(dense_unsigned_array const&) = delete;
  dense_unsigned_array(
    dense_unsigned_array&& rhs) noexcept
    : buffer(std::move(rhs))
    , element_byte_size_(rhs.element_byte_size_)
    , endian_(rhs.endian_)
  {
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0;)
  }

  auto operator=(dense_unsigned_array const&) -> dense_unsigned_array& = delete;
  auto operator=(
    dense_unsigned_array&& rhs) noexcept -> dense_unsigned_array&
  {
    buffer::operator=(std::move(rhs));
    element_byte_size_ = rhs.element_byte_size_;
    endian_            = rhs.endian_;
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0;)
    return *this;
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

  template<integral_c Count>
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

  template<integral_c Index>
  [[nodiscard]] auto get(
    Index index) const -> T
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    return dense_unsigned_ops::get<T>(data(), index64, element_bytes(), endian());
  }

  template<integral_c Index>
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
  u8          element_byte_size_ = 0;
  std::endian endian_            = std::endian::native;
  IF(KASPAN_DEBUG,
     u64 size_ = 0);
};

template<unsigned_c   T = u64,
         integral_c Count>
auto
make_dense_unsigned_array(
  Count       count,
  u8          element_byte_size,
  std::endian endian = std::endian::native) -> dense_unsigned_array<T>
{
  return dense_unsigned_array<T>{ count, element_byte_size, endian };
}

} // namespace kaspan
