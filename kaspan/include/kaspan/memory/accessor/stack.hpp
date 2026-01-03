#pragma once

#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/memory/accessor/stack_mixin.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <type_traits>
#include <utility>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class stack final
  : public buffer
  , public stack_mixin<stack<T>, T>
{
public:
  stack() noexcept = default;
  ~stack()         = default;

  explicit stack(u64 count) noexcept(false)
    : buffer(count * sizeof(T))
  {
  }

  stack(stack const&) = delete;
  stack(stack&& rhs) noexcept
    : buffer(std::move(rhs))
    , end_(rhs.end_)
  {
    rhs.end_ = 0;
  }

  auto operator=(stack const&) -> stack& = delete;
  auto operator=(stack&& rhs) noexcept -> stack&
  {
    buffer::operator=(std::move(rhs));
    end_     = rhs.end_;
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

  void set_size(u64 size)
  {
    end_ = size;
  }

  [[nodiscard]] explicit operator stack_accessor<T>() const noexcept
  {
    return stack_accessor<T>{ data(), end_ };
  }

private:
  u64 end_ = 0;
};

template<typename T>
auto
make_stack(u64 size) -> stack<T>
{
  return stack<T>{ size };
}

} // namespace kaspan
