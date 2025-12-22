#pragma once

#include <debug/assert.hpp>
#include <memory/accessor/stack_mixin.hpp>
#include <memory/borrow.hpp>
#include <util/arithmetic.hpp>

#include <cstdlib>
#include <type_traits>

template<typename T>
  requires(std::is_trivially_copyable_v<T> and
           std::is_trivially_constructible_v<T> and
           std::is_trivially_destructible_v<T>)
class StackAccessor final : public StackMixin<StackAccessor<T>, T>
{
public:
  using value_type = T;

  explicit StackAccessor(void* data)
    : data_(data)
  {
  }

  StackAccessor()  = default;
  ~StackAccessor() = default;

  StackAccessor(StackAccessor const& rhs) noexcept = default;
  StackAccessor(StackAccessor&& rhs) noexcept      = default;

  auto operator=(StackAccessor const& rhs) noexcept -> StackAccessor& = default;
  auto operator=(StackAccessor&& rhs) noexcept -> StackAccessor&      = default;

  static auto borrow(void*& memory, u64 count) noexcept -> StackAccessor
  {
    return StackAccessor{ ::borrow<T>(memory, count) };
  }

  auto size() const -> u64
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

  auto data() const -> T const*
  {
    return static_cast<T*>(data_);
  }

private:
  void* data_ = nullptr;
  u64   end_  = 0;
};
