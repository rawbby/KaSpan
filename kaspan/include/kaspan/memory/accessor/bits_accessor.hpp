#pragma once

#include <kaspan/memory/accessor/bits_ops.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/math.hpp>

namespace kaspan {

class bits_accessor final
{
public:
  template<arithmetic_concept Size>
  explicit bits_accessor(void* data, Size size)
    : data_(size == 0 ? nullptr : data)
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = static_cast<u64>(size));
    KASPAN_VALGRIND_CHECK_MEM_IS_ADDRESSABLE(data, round_up<64>(size) / 8);
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

  template<arithmetic_concept Size>
  void clear(Size end)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    auto const end64 = static_cast<u64>(end);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end64, 0, size_);
    bits_ops::clear(data(), end64);
  }

  template<arithmetic_concept Size>
  void fill(Size end)
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    auto const end64 = static_cast<u64>(end);
    DEBUG_ASSERT_IN_RANGE_INCLUSIVE(end64, 0, size_);
    bits_ops::fill(data(), end64);
  }

  template<arithmetic_concept Index>
  [[nodiscard]] auto get(Index index) const -> bool
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = static_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    return bits_ops::get(data(), index64);
  }

  template<arithmetic_concept Index>
  void set(Index index, bool value)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = static_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    bits_ops::set(data(), index64, value);
  }

  template<arithmetic_concept Index>
  void set(Index index)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = static_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    bits_ops::set(data(), index64);
  }

  template<arithmetic_concept Index>
  void unset(Index index)
  {
    DEBUG_ASSERT_GE(index, 0);
    DEBUG_ASSERT_LE(index, std::numeric_limits<u64>::max());
    auto const index64 = static_cast<u64>(index);
    DEBUG_ASSERT_LT(index64, size_);
    bits_ops::unset(data(), index64);
  }

  template<arithmetic_concept Index = size_t>
  void for_each(Index end, std::invocable<Index> auto&& fn) const
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::for_each<Index>(data(), end, std::forward<decltype(fn)>(fn));
  }

  template<arithmetic_concept Index = size_t>
  void set_each(Index end, std::invocable<Index> auto&& fn)
  {
    DEBUG_ASSERT_LE(end, size_);
    bits_ops::set_each<Index>(data(), end, std::forward<decltype(fn)>(fn));
  }

private:
  void* data_ = nullptr;
  IF(KASPAN_DEBUG, u64 size_ = 0);
};

template<arithmetic_concept Size>
auto
borrow_bits(void** memory, Size size) noexcept -> bits_accessor
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = static_cast<u64>(size);
  return bits_accessor{ borrow_array<u64>(memory, ceildiv<64>(size64)), size64 };
}

template<arithmetic_concept Size>
auto
borrow_bits_clean(void** memory, Size size) noexcept -> bits_accessor
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = static_cast<u64>(size);
  return bits_accessor{ borrow_array_clean<u64>(memory, ceildiv<64>(size64)), size64 };
}

template<arithmetic_concept Size>
auto
borrow_bits_filled(void** memory, Size size) noexcept -> bits_accessor
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = static_cast<u64>(size);
  return bits_accessor{ borrow_array_filled<u64>(memory, ceildiv<64>(size64)), size64 };
}

template<arithmetic_concept Size>
auto
view_bits(void* data, Size size) noexcept -> bits_accessor
{
  return bits_accessor{ data, size };
}

} // namespace kaspan
