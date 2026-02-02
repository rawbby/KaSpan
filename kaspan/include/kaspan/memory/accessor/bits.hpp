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

  template<arithmetic_concept Size>
  explicit bits(Size size) noexcept(false)
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

  template<arithmetic_concept Size>
  void clear(
    Size end)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    auto const end64 = integral_cast<u64>(end);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end64, 0, size_);
    bits_ops::clear(data(), end64);
  }

  template<arithmetic_concept Size>
  void fill(
    Size end)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    auto const end64 = integral_cast<u64>(end);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end64, 0, size_);
    bits_ops::fill(data(), end64);
  }

  template<arithmetic_concept Index>
  [[nodiscard]] auto get(
    Index index) const -> bool
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    return bits_ops::get(data(), index64);
  }

  template<arithmetic_concept Index>
  void set(
    Index index,
    bool  value)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    bits_ops::set(data(), index64, value);
  }

  template<arithmetic_concept Index>
  void set(
    Index index)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    bits_ops::set(data(), index64);
  }

  template<arithmetic_concept Index>
  void unset(
    Index index)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = integral_cast<u64>(index);
    DEBUG_ASSERT_IN_RANGE(index64, 0, size_);
    bits_ops::unset(data(), index64);
  }

  template<arithmetic_concept Index = size_t>
  void each(
    Index                        end,
    std::invocable<Index> auto&& fn) const
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::each<Index>(data(), end, std::forward<decltype(fn)>(fn));
  }

  template<arithmetic_concept Index = size_t>
  void set_each(
    Index                        end,
    std::invocable<Index> auto&& fn)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::set_each<Index>(data(), end, std::forward<decltype(fn)>(fn));
  }

private:
  IF(KASPAN_DEBUG,
     u64 size_);
};

template<arithmetic_concept Size>
auto
make_bits(
  Size size) noexcept -> bits
{
  return bits{ size };
}

template<arithmetic_concept Size>
auto
make_bits_clean(
  Size size) noexcept -> bits
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

template<arithmetic_concept Size>
auto
make_bits_filled(
  Size size) noexcept -> bits
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
