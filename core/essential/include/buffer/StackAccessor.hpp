#pragma once

#include "debug/Assert.hpp"

#include <util/Arithmetic.hpp>
#include <util/Util.hpp>

#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <type_traits>

template<typename T>
  requires(std::is_trivially_copyable_v<T> and
           std::is_trivially_constructible_v<T> and
           std::is_trivially_destructible_v<T>)
class StackAccessor final
{
public:
  using value_type = T;

  explicit StackAccessor(void* __restrict data)
    : data_(data)
    , end_(0)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    DEBUG_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(data) % alignof(T), 0);
  }

  StackAccessor()  = delete;
  ~StackAccessor() = default;

  StackAccessor(StackAccessor const& rhs) noexcept = default;
  StackAccessor(StackAccessor&& rhs) noexcept      = default;

  auto operator=(StackAccessor const& rhs) noexcept -> StackAccessor& = default;
  auto operator=(StackAccessor&& rhs) noexcept -> StackAccessor&      = default;

  static auto borrow(void* __restrict& memory, u64 n) noexcept -> StackAccessor
  {
    DEBUG_ASSERT(page_aligned(memory));
    auto const result    = StackAccessor{ memory };
    auto const byte_size = page_ceil(n * sizeof(T));
    memory               = static_cast<void*>(static_cast<std::byte*>(memory) + byte_size);
    return result;
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

private:
  void* data_;
  u64   end_;
};
