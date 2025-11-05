#pragma once

#include <debug/assert.hpp>
#include <memory/buffer.hpp>
#include <util/arithmetic.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

template<typename T = byte>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
static auto
borrow(void* & memory, u64 count) noexcept -> T*
{
  DEBUG_ASSERT_NE(memory, nullptr);
  DEBUG_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(memory) % alignof(T), 0);
  DEBUG_ASSERT(is_page_aligned(memory));
  auto const result    = static_cast<T*>(memory);
  auto const byte_size = page_ceil(count * sizeof(T));
  memory               = static_cast<void*>(static_cast<std::byte*>(memory) + byte_size);
  return result;
}

template<typename T = byte>
static auto
borrow_clean(void* & memory, u64 count) noexcept -> T*
{
  auto result = borrow<T>(memory, count);
  std::memset(result, 0, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
borrow_filled(void* & memory, u64 count) noexcept -> T*
{
  auto result = borrow<T>(memory, count);
  std::memset(result, ~0, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
borrow_filled(void* & memory, T const& value, u64 count) noexcept -> T*
{
  auto result = borrow<T>(memory, count);
  for (u64 i = 0; i < count; ++i) {
    std::memcpy(&result[i], &value, sizeof(T));
  }
  return result;
}
