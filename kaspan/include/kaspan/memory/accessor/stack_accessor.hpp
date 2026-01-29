#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/pp.hpp>

#include <limits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
class stack_accessor final
{
public:
  stack_accessor()  = default;
  ~stack_accessor() = default;

  template<arithmetic_concept Size>
  explicit stack_accessor(
    void* data,
    Size  size)
    : data_(size == 0 ? nullptr : data)
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = integral_cast<u64>(size));
  }

  template<arithmetic_concept End,
           arithmetic_concept Size>
  stack_accessor(
    void* data,
    End   end,
    Size  size)
    : data_(size == 0 ? nullptr : data)
    , end_(integral_cast<u64>(end))
  {
    DEBUG_ASSERT_GE(end, 0);
    DEBUG_ASSERT_LE(end, std::numeric_limits<u64>::max());
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = integral_cast<u64>(size));
  }

  stack_accessor(stack_accessor const& rhs) noexcept = default;
  stack_accessor(stack_accessor&& rhs) noexcept      = default;

  auto operator=(stack_accessor const& rhs) noexcept -> stack_accessor& = default;
  auto operator=(stack_accessor&& rhs) noexcept -> stack_accessor&      = default;

  [[nodiscard]] auto size() const -> u64
  {
    return end_;
  }

  [[nodiscard]] auto empty() const -> bool
  {
    return end_ == 0;
  }

  auto begin() const
  {
    return data();
  }

  auto end() const
  {
    return data() + end_;
  }

  auto begin()
  {
    return data();
  }

  auto end()
  {
    return data() + end_;
  }

  void clear()
  {
    end_ = 0;
  }

  auto data() -> T*
  {
    return static_cast<T*>(data_);
  }

  [[nodiscard]] auto data() const -> T const*
  {
    return static_cast<T const*>(data_);
  }

  void push(
    T t) noexcept
  {
    DEBUG_ASSERT_LT(end_, size_);
    data()[end_++] = t;
  }

  auto back() -> T&
  {
    DEBUG_ASSERT_GT(size(), 0);
    return data()[end_ - 1];
  }

  void pop()
  {
    DEBUG_ASSERT_GT(size(), 0);
    --end_;
  }

  T pop_back()
  {
    T item = back();
    pop();
    return item;
  }

private:
  void* data_ = nullptr;
  u64   end_  = 0;
  IF(KASPAN_DEBUG,
     u64 size_ = 0);
};

template<typename T,
         arithmetic_concept Size>
auto
borrow_stack(
  void** memory,
  Size   size) -> stack_accessor<T>
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = integral_cast<u64>(size);
  return stack_accessor<T>{ borrow_array<T>(memory, size64), size64 };
}

template<typename T,
         arithmetic_concept Size>
auto
view_stack(
  void* data,
  Size  size) -> stack_accessor<T>
{
  return stack_accessor<T>{ data, size };
}

} // namespace kaspan
