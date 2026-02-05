#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/accessor/bits_ops.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/math.hpp>
#include <kaspan/util/pp.hpp>

#include <concepts>
#include <limits>
#include <stddef.h>
#include <utility>

namespace kaspan {

class bits_accessor final
{
public:
  explicit bits_accessor(
    void*                                    data,
    [[maybe_unused]] integral_c auto size)
    : data_(data)
  {
    IF(KASPAN_DEBUG, size_ = integral_cast<u64>(size));
  }

  bits_accessor()  = default;
  ~bits_accessor() = default;

  bits_accessor(bits_accessor const& rhs) noexcept = default;
  bits_accessor(bits_accessor&& rhs) noexcept      = default;

  auto operator=(bits_accessor const& rhs) noexcept -> bits_accessor& = default;
  auto operator=(bits_accessor&& rhs) noexcept -> bits_accessor&      = default;

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(data_);
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(data_);
  }

  void clear(
    integral_c auto end)
  {
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end, 0, size_);
    bits_ops::clear(data(), end);
  }

  void fill(
    integral_c auto end)
  {
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end, 0, size_);
    bits_ops::fill(data(), end);
  }

  [[nodiscard]] auto get(
    integral_c auto index) const -> bool
  {
    DEBUG_ASSERT_LT(index, size_);
    return bits_ops::get(data(), index);
  }

  void set(
    integral_c auto index,
    bool                    value)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::set(data(), index, value);
  }

  void set(
    integral_c auto index)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::set(data(), index);
  }

  void unset(
    integral_c auto index)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::unset(data(), index);
  }

  auto getset(
    integral_c auto index,
    bool                    value) -> bool
  {
    DEBUG_ASSERT_LT(index, size_);
    return bits_ops::getset(data(), index, value);
  }

  auto getset(
    integral_c auto index) -> bool
  {
    DEBUG_ASSERT_LT(index, size_);
    return bits_ops::getset(data(), index);
  }

  auto getunset(
    integral_c auto index) -> bool
  {
    DEBUG_ASSERT_LT(index, size_);
    return bits_ops::getunset(data(), index);
  }

  template<typename Consumer>
  void each(
    integral_c auto end,
    Consumer&&              fn) const
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::each(data(), end, std::forward<Consumer>(fn));
  }

  template<typename Consumer>
  void set_each(
    integral_c auto end,
    Consumer&&              fn)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::set_each(data(), end, std::forward<Consumer>(fn));
  }

private:
  void* data_ = nullptr;
  IF(KASPAN_DEBUG,
     u64 size_ = 0);
};

auto
borrow_bits(
  void**                  memory,
  integral_c auto size) noexcept -> bits_accessor
{
  DEBUG_ASSERT_GE(size, 0);
  return bits_accessor{ borrow_array<u64>(memory, ceildiv<64>(size)), size };
}

auto
borrow_bits_clean(
  void**                  memory,
  integral_c auto size) noexcept -> bits_accessor
{
  DEBUG_ASSERT_GE(size, 0);
  return bits_accessor{ borrow_array_clean<u64>(memory, ceildiv<64>(size)), size };
}

auto
borrow_bits_filled(
  void**                  memory,
  integral_c auto size) noexcept -> bits_accessor
{
  DEBUG_ASSERT_GE(size, 0);
  return bits_accessor{ borrow_array_filled<u64>(memory, ceildiv<64>(size)), size };
}

auto
view_bits(
  void*                   data,
  integral_c auto count) noexcept -> bits_accessor
{
  return bits_accessor{ data, count };
}

auto
view_bits_clean(
  void*                   data,
  integral_c auto count) noexcept -> bits_accessor
{
  auto accessor = bits_accessor{ data, count };
  accessor.clear(count);
  return accessor;
}

} // namespace kaspan
