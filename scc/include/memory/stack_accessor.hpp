#pragma once

#include <debug/assert.hpp>
#include <memory/borrow.hpp>
#include <util/arithmetic.hpp>

#include <cstdlib>
#include <type_traits>

template<typename T>
  requires(std::is_trivially_copyable_v<T> and
           std::is_trivially_constructible_v<T> and
           std::is_trivially_destructible_v<T>)
class StackAccessor final
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

  static auto create(u64 count) noexcept -> std::pair<Buffer, StackAccessor>
  {
    auto  buffer = Buffer::create<T>(count);
    void* memory = buffer.date();
    return { std::move(buffer), StackAccessor{ memory } };
  }

  [[nodiscard]] auto operator[](size_t index) -> T&
  {
    return data()[index];
  }

  [[nodiscard]] auto operator[](size_t index) const -> T const&
  {
    return data()[index];
  }

  void clear()
  {
    end_ = 0;
  }

  void push(T const& t) noexcept
  {
    std::memcpy(&data()[end_++], &t, sizeof(T));
  }

  void push(T&& t) noexcept
  {
    std::memcpy(&data()[end_++], &t, sizeof(T));
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
  }

  auto size() const -> u64
  {
    return end_;
  }

  auto empty() const -> bool
  {
    return end_ == static_cast<u64>(0);
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
