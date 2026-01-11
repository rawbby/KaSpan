#pragma once

#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstring>
#include <type_traits>
#include <utility>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class stack final : public buffer
{
public:
  stack() noexcept = default;
  ~stack()         = default;

  template<arithmetic_concept Size>
  explicit stack(Size size) noexcept(false)
    : buffer(static_cast<u64>(size) * sizeof(T))
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    IF(KASPAN_DEBUG, size_ = static_cast<u64>(size));
  }

  stack(stack const&) = delete;
  stack(stack&& rhs) noexcept
    : buffer(std::move(rhs))
    , end_(rhs.end_)
  {
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0;)
    rhs.end_ = 0;
  }

  auto operator=(stack const&) -> stack& = delete;
  auto operator=(stack&& rhs) noexcept -> stack&
  {
    buffer::operator=(std::move(rhs));
    end_ = rhs.end_;
    IF(KASPAN_DEBUG, size_ = rhs.size_;)
    IF(KASPAN_DEBUG, rhs.size_ = 0;)
    rhs.end_ = 0;
    return *this;
  }

  [[nodiscard]] auto data() -> T*
  {
    return static_cast<T*>(buffer::data());
  }

  [[nodiscard]] auto data() const -> T const*
  {
    return static_cast<T const*>(buffer::data());
  }

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
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(data(), end_ * sizeof(T));
    end_ = 0;
  }

  void push(T item) noexcept
  {
    DEBUG_ASSERT_LT(end_, size_);
    data()[end_] = item;
    ++end_;
  }

  auto back() -> T&
  {
    DEBUG_ASSERT_GT(end_, 0);
    return data()[end_ - 1];
  }

  void pop()
  {
    DEBUG_ASSERT_GT(end_, 0);
    --end_;
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(data() + end_, sizeof(T));
  }

  T pop_back()
  {
    T item = back();
    pop();
    return item;
  }

private:
  u64 end_ = 0;
  IF(KASPAN_DEBUG, u64 size_ = 0);
};

template<typename T, arithmetic_concept Size>
auto
make_stack(Size size) -> stack<T>
{
  return stack<T>{ size };
}

} // namespace kaspan
