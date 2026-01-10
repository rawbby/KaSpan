#pragma once

#include <kaspan/memory/accessor/bits_ops.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/math.hpp>

namespace kaspan {

class bits_accessor final
{
public:
  explicit bits_accessor(void* data, u64 size)
    : data_(size == 0 ? nullptr : data)
  {
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = size);
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
    DEBUG_ASSERT_LT(index, size_);
    return bits_ops::get(data(), index);
  }

  void set(u64 index, bool value)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::set(data(), index, value);
  }

  void set(u64 index)
  {
    DEBUG_ASSERT_LT(index, size_);
    bits_ops::set(data(), index);
  }

  void unset(u64 index)
  {
    DEBUG_ASSERT_LT(index, size_);
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
  void* data_ = nullptr;
  IF(KASPAN_DEBUG, u64 size_ = 0);
};

inline auto
borrow_bits(void** memory, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ borrow_array<u64>(memory, ceildiv<64>(size)), size };
}

inline auto
borrow_bits_clean(void** memory, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ borrow_array_clean<u64>(memory, ceildiv<64>(size)), size };
}

inline auto
borrow_bits_filled(void** memory, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ borrow_array_filled<u64>(memory, ceildiv<64>(size)), size };
}

inline auto
view_bits(void* data, u64 size) noexcept -> bits_accessor
{
  return bits_accessor{ data, size };
}

} // namespace kaspan
