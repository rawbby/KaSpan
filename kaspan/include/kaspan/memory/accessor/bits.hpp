#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/memory/accessor/bits_ops.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/math.hpp>
#include <kaspan/util/pp.hpp>

#include <cstring>
#include <utility>

namespace kaspan {

class bits final
{
public:
  bits() noexcept = default;
  explicit bits(arithmetic_concept auto size) noexcept(false)
    : data_(line_alloc<u64>(ceildiv<64>(integral_cast<u64>(size)) * sizeof(u64)))
  {
    IF(KASPAN_DEBUG, size_ = round_up<64>(integral_cast<u64>(size)));
  }

  ~bits()
  {
    line_free(data_);
  }

  bits(bits const&) = delete;
  bits(
    bits&& rhs) noexcept
    : data_(rhs.data_)
  {
    rhs.data_ = nullptr;
    IF(KASPAN_DEBUG, size_ = rhs.size_; rhs.size_ = 0);
  }

  auto operator=(bits const&) -> bits& = delete;
  auto operator=(
    bits&& rhs) noexcept -> bits&
  {
    data_     = rhs.data_;
    rhs.data_ = nullptr;
    IF(KASPAN_DEBUG, size_ = rhs.size_; rhs.size_ = 0);
    return *this;
  }

  [[nodiscard]] auto data() -> u64*
  {
    return data_;
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return data_;
  }

  void clear(
    arithmetic_concept auto end)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::clear(data(), end);
  }

  void fill(
    arithmetic_concept auto end)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::fill(data(), end);
  }

  [[nodiscard]] auto get(
    arithmetic_concept auto index) const -> bool
  {
    DEBUG_ASSERT_LT(index, size_);
    return bits_ops::get(data(), index);
  }

  void set(
    arithmetic_concept auto index,
    bool                    value)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::set(data(), index, value);
  }

  void set(
    arithmetic_concept auto index)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::set(data(), index);
  }

  void unset(
    arithmetic_concept auto index)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::unset(data(), index);
  }

  template<typename Consumer>
  void each(
    arithmetic_concept auto end,
    Consumer&&              fn) const
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::each(data(), end, std::forward<Consumer>(fn));
  }

  template<typename Consumer>
  void set_each(
    arithmetic_concept auto end,
    Consumer&&              fn)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::set_each(data(), end, std::forward<Consumer>(fn));
  }

private:
  u64* data_ = nullptr;
  IF(KASPAN_DEBUG,
     u64 size_ = 0);
};

auto
make_bits(
  arithmetic_concept auto size) noexcept -> bits
{
  return bits{ size };
}

auto
make_bits_clean(
  arithmetic_concept auto size) noexcept -> bits
{
  bits res{ size };
  res.clear(size);
  return res;
}

auto
make_bits_filled(
  arithmetic_concept auto size) noexcept -> bits
{
  bits res{ size };
  res.fill(size);
  return res;
}

} // namespace kaspan
