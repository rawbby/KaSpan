#pragma once

#include <memory/accessor/stack_mixin.hpp>
#include <memory/buffer.hpp>
#include <util/arithmetic.hpp>

#include <cstdlib>
#include <type_traits>
#include <utility>

template<typename T>
  requires(std::is_trivially_copyable_v<T> and
           std::is_trivially_constructible_v<T> and
           std::is_trivially_destructible_v<T>)
class Stack final : public Buffer, public StackMixin<Stack<T>, T>
{
public:
  using value_type = T;

  Stack() noexcept = default;
  ~Stack()         = default;

  explicit Stack(u64 count) noexcept(false)
    : Buffer(count * sizeof(T))
  {
  }

  Stack(Stack const&) = delete;
  Stack(Stack&& rhs) noexcept
    : Buffer(std::move(rhs))
    , end_(rhs.end_)
  {
    rhs.end_ = 0;
  }

  auto operator=(Stack const&) -> Stack& = delete;
  auto operator=(Stack&& rhs) noexcept -> Stack&
  {
    Buffer::operator=(std::move(rhs));
    end_     = rhs.end_;
    rhs.end_ = 0;
    return *this;
  }

  static auto create(u64 count) noexcept(false) -> Stack
  {
    return Stack{ count };
  }

  [[nodiscard]] auto data() -> T*
  {
    return static_cast<T*>(Buffer::data());
  }

  [[nodiscard]] auto data() const -> T const*
  {
    return static_cast<T const*>(Buffer::data());
  }

  auto size() const -> u64
  {
    return end_;
  }

  void set_size(u64 size)
  {
    end_ = size;
  }

private:
  u64 end_ = 0;
};
