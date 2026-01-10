#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstring>
#include <type_traits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class stack_accessor final
{
public:
  stack_accessor()  = default;
  ~stack_accessor() = default;

  explicit stack_accessor(void* data, u64 size)
    : data_(size == 0 ? nullptr : data)
  {
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = size);
  }

  stack_accessor(void* data, u64 end, u64 size)
    : data_(size == 0 ? nullptr : data)
    , end_(end)
  {
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = size);
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

  void push(T t) noexcept
  {
    IF(KASPAN_DEBUG, DEBUG_ASSERT_LT(end_, size_);)
    KASPAN_VALGRIND_MAKE_MEM_DEFINED(&data()[end_], sizeof(T));
    std::memcpy(&data()[end_], &t, sizeof(T));
    ++end_;
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
  IF(KASPAN_DEBUG, u64 size_ = 0);
};

template<typename T>
auto
borrow_stack(void** memory, u64 size) -> stack_accessor<T>
{
  return stack_accessor<T>{ borrow_array<T>(memory, size), size };
}

template<typename T>
auto
view_stack(void* data, u64 size) -> stack_accessor<T>
{
  return stack_accessor<T>{ data, size };
}

} // namespace kaspan
