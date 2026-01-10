#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/bits_ops.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/math.hpp>

#include <cstring>
#include <utility>

namespace kaspan {

class bits final : public buffer
{
public:
  bits() noexcept = default;
  ~bits()         = default;

  explicit bits(u64 size) noexcept(false)
    : buffer(ceildiv<64>(size) * sizeof(u64))
  {
    IF(KASPAN_DEBUG, size_ = round_up<64>(size));
  }

  bits(bits const&) = delete;
  bits(bits&& rhs) noexcept
    : buffer(std::move(rhs))
  {
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0);
  }

  auto operator=(bits const&) -> bits& = delete;
  auto operator=(bits&& rhs) noexcept -> bits&
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

  void clear(u64 end)
  {
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end, 0, size_);
    bits_ops::clear(data(), end);
  }

  void fill(u64 end)
  {
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end, 0, size_);
    bits_ops::fill(data(), end);
  }

  [[nodiscard]] auto get(u64 index) const -> bool
  {
    DEBUG_ASSERT_IN_RANGE(index, 0, size_);
    return bits_ops::get(data(), index);
  }

  void set(u64 index, bool value)
  {
    DEBUG_ASSERT_IN_RANGE(index, 0, size_);
    bits_ops::set(data(), index, value);
  }

  void set(u64 index)
  {
    DEBUG_ASSERT_IN_RANGE(index, 0, size_);
    bits_ops::set(data(), index);
  }

  void unset(u64 index)
  {
    DEBUG_ASSERT_IN_RANGE(index, 0, size_);
    bits_ops::unset(data(), index);
  }

  template<ArithmeticConcept Index = size_t>
  void for_each(Index end, std::invocable<Index> auto&& fn) const
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::for_each<Index>(data(), end, std::forward<decltype(fn)>(fn));
  }

  template<ArithmeticConcept Index = size_t>
  void set_each(Index end, std::invocable<Index> auto&& fn)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::set_each<Index>(data(), end, std::forward<decltype(fn)>(fn));
  }

private:
  IF(KASPAN_DEBUG, u64 size_);
};

inline auto
make_bits(u64 size) noexcept -> bits
{
  return bits{ size };
}

inline auto
make_bits_clean(u64 size) noexcept -> bits
{
  bits res{ size };
  res.clear(round_up<64>(size));

  if constexpr (KASPAN_DEBUG) {
    bits_ops::for_each(res.data(), size, [](u64 /* i */) {
      ASSERT(false, "this code should be unreachable");
    });
    for (u64 i = 0; i < size; ++i) {
      ASSERT_EQ(res.get(i), false);
    }
  }

  return res;
}

inline auto
make_bits_filled(u64 size) noexcept -> bits
{
  bits res{ size };
  res.fill(round_up<64>(size));

  if constexpr (KASPAN_DEBUG) {
    u64 c = 0;
    bits_ops::for_each(res.data(), size, [&c](u64 i) {
      ASSERT_EQ(i, c++);
    });
    for (u64 i = 0; i < size; ++i) {
      ASSERT_EQ(res.get(i), true);
    }
  }

  return res;
}

} // namespace kaspan
