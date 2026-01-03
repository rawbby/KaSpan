#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/stack_mixin.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstdlib>
#include <type_traits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class stack_accessor final : public stack_mixin<stack_accessor<T>, T>
{
public:
  stack_accessor()  = default;
  ~stack_accessor() = default;

  explicit stack_accessor(void* data)
    : data_(data)
  {
  }

  stack_accessor(void* data, u64 end)
    : data_(data)
    , end_(end)
  {
  }

  stack_accessor(stack_accessor const& rhs) noexcept = default;
  stack_accessor(stack_accessor&& rhs) noexcept      = default;

  auto operator=(stack_accessor const& rhs) noexcept -> stack_accessor& = default;
  auto operator=(stack_accessor&& rhs) noexcept -> stack_accessor&      = default;

  static auto borrow(void** memory, u64 count) noexcept -> stack_accessor
  {
    return stack_accessor{ kaspan::borrow_array<T>(memory, count) };
  }

  [[nodiscard]] auto size() const -> u64
  {
    return end_;
  }

  void set_size(u64 size)
  {
    end_ = size;
  }

  auto data() -> T*
  {
    return static_cast<T*>(data_);
  }

  [[nodiscard]] auto data() const -> T const*
  {
    return static_cast<T*>(data_);
  }

private:
  void* data_ = nullptr;
  u64   end_  = 0;
};

template<typename T>
auto
borrow_stack(void** memory, u64 size) -> stack_accessor<T>
{
  return stack_accessor<T>{ borrow_array<T>(memory, size) };
}

} // namespace kaspan
