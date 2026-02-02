#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/memory/accessor/bits_ops.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/math.hpp>
#include <kaspan/util/pp.hpp>

#include <concepts>
#include <cstring>
#include <limits>
#include <utility>

namespace kaspan {

class bits final : public buffer
{
public:
  bits() noexcept = default;
  ~bits()         = default;

  explicit bits(arithmetic_concept auto size) noexcept(false)
    : buffer(ceildiv<64>(integral_cast<u64>(size)) * sizeof(u64))
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    IF(KASPAN_DEBUG, size_ = round_up<64>(integral_cast<u64>(size)));
  }

  bits(bits const&) = delete;
  bits(
    bits&& rhs) noexcept
    : buffer(std::move(rhs))
  {
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0);
  }

  auto operator=(bits const&) -> bits& = delete;
  auto operator=(
    bits&& rhs) noexcept -> bits&
  {
    buffer::operator=(std::move(rhs));
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0);
    return *this;
  }

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(buffer::data());
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(buffer::data());
  }

  void clear(
    arithmetic_concept auto end)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    auto const end64 = integral_cast<u64>(end);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end64, 0, size_);
    bits_ops::clear(data(), end64);
  }

  void fill(
    arithmetic_concept auto end)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    auto const end64 = integral_cast<u64>(end);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end64, 0, size_);
    bits_ops::fill(data(), end64);
  }

  [[nodiscard]] auto get(
    arithmetic_concept auto index) const -> bool
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    return bits_ops::get(data(), index64);
  }

  void set(
    arithmetic_concept auto index,
    bool                    value)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    bits_ops::set(data(), index64, value);
  }

  void set(
    arithmetic_concept auto index)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    bits_ops::set(data(), index64);
  }

  void unset(
    arithmetic_concept auto index)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    bits_ops::unset(data(), index64);
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
  IF(KASPAN_DEBUG,
     u64 size_);
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
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = integral_cast<u64>(size);
  bits       res{ size64 };
  res.clear(round_up<64>(size64));

  if constexpr (KASPAN_DEBUG) {
    bits_ops::each(res.data(), size64, [](u64 /* i */) { ASSERT(false, "this code should be unreachable"); });
    for (u64 i = 0; i < size64; ++i) {
      ASSERT_EQ(res.get(i), false);
    }
  }

  return res;
}

auto
make_bits_filled(
  arithmetic_concept auto size) noexcept -> bits
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = integral_cast<u64>(size);
  bits       res{ size64 };
  res.fill(round_up<64>(size64));

  if constexpr (KASPAN_DEBUG) {
    u64 c = 0;
    bits_ops::each(res.data(), size64, [&c](u64 i) { ASSERT_EQ(i, c++); });
    for (u64 i = 0; i < size64; ++i) {
      ASSERT_EQ(res.get(i), true);
    }
  }

  return res;
}

} // namespace kaspan
