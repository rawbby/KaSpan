#pragma once

#include <debug/assert.hpp>
#include <debug/valgrind.hpp>
#include <memory/buffer.hpp>
#include <memory/line.hpp>
#include <util/arithmetic.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class Array : public Buffer
{
public:
  Array() noexcept = default;

  explicit Array(u64 count) noexcept(false)
    : Buffer(count * sizeof(T))
  {
  }

  Array(Array const&) = delete;
  Array(Array&& rhs) noexcept
    : Buffer(std::move(rhs))
  {
  }

  auto operator=(Array const&) -> Array& = delete;
  auto operator=(Array&& rhs) noexcept -> Array&
  {
    if (this != &rhs) {
      Buffer::operator=(std::move(rhs));
    }
    return *this;
  }

  [[nodiscard]] auto operator*() const noexcept -> T&
  {
    return *data();
  }

  [[nodiscard]] auto operator->() const noexcept -> T*
  {
    return data();
  }

  [[nodiscard]] auto operator[](u64 idx) const noexcept -> T&
  {
    return data()[idx];
  }

  [[nodiscard]] auto operator+(std::ptrdiff_t offset) const noexcept -> T*
  {
    return data() + offset;
  }

  [[nodiscard]] auto operator-(std::ptrdiff_t offset) const noexcept -> T*
  {
    return data() - offset;
  }

  auto operator+=(std::ptrdiff_t offset) noexcept -> Array&
  {
    data_ = static_cast<void*>(data() + offset);
    return *this;
  }

  auto operator-=(std::ptrdiff_t offset) noexcept -> Array&
  {
    data_ = static_cast<void*>(data() - offset);
    return *this;
  }

  auto operator++() noexcept -> Array&
  {
    data_ = static_cast<void*>(data() + 1);
    return *this;
  }

  auto operator--() noexcept -> Array&
  {
    data_ = static_cast<void*>(data() - 1);
    return *this;
  }

  auto operator++(int) noexcept -> T*
  {
    T* tmp = data();
    data_  = static_cast<void*>(data() + 1);
    return tmp;
  }

  auto operator--(int) noexcept -> T*
  {
    T* tmp = data();
    data_  = static_cast<void*>(data() - 1);
    return tmp;
  }

  [[nodiscard]] friend auto operator==(Array const& lhs, Array const& rhs) noexcept -> bool
  {
    return lhs.data() == rhs.data();
  }

  [[nodiscard]] friend auto operator!=(Array const& lhs, Array const& rhs) noexcept -> bool
  {
    return lhs.data() != rhs.data();
  }

  [[nodiscard]] friend auto operator<(Array const& lhs, Array const& rhs) noexcept -> bool
  {
    return lhs.data() < rhs.data();
  }

  [[nodiscard]] friend auto operator<=(Array const& lhs, Array const& rhs) noexcept -> bool
  {
    return lhs.data() <= rhs.data();
  }

  [[nodiscard]] friend auto operator>(Array const& lhs, Array const& rhs) noexcept -> bool
  {
    return lhs.data() > rhs.data();
  }

  [[nodiscard]] friend auto operator>=(Array const& lhs, Array const& rhs) noexcept -> bool
  {
    return lhs.data() >= rhs.data();
  }

  [[nodiscard]] friend auto operator==(Array const& arr, std::nullptr_t) noexcept -> bool
  {
    return arr.data() == nullptr;
  }

  [[nodiscard]] friend auto operator==(std::nullptr_t, Array const& arr) noexcept -> bool
  {
    return arr.data() == nullptr;
  }

  [[nodiscard]] friend auto operator!=(Array const& arr, std::nullptr_t) noexcept -> bool
  {
    return arr.data() != nullptr;
  }

  [[nodiscard]] friend auto operator!=(std::nullptr_t, Array const& arr) noexcept -> bool
  {
    return arr.data() != nullptr;
  }

  [[nodiscard]] friend auto operator+(std::ptrdiff_t offset, Array const& arr) noexcept -> T*
  {
    return arr.data() + offset;
  }

  [[nodiscard]] operator T*() const noexcept
  {
    return data();
  }

  [[nodiscard]] operator T const*() const noexcept
  {
    return data();
  }

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return data_ != nullptr;
  }

  [[nodiscard]] auto data() const noexcept -> T*
  {
    return static_cast<T*>(data_);
  }
};

template<typename T = byte>
static auto
make_array(u64 count) noexcept -> Array<T>
{
  return Array<T>{ count };
}

template<typename T = byte>
static auto
make_array_clean(u64 count) noexcept -> Array<T>
{
  auto result = Array<T>{ count };
  std::memset(result, 0x00, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
make_array_filled(u64 count) noexcept -> Array<T>
{
  auto result = Array<T>{ count };
  std::memset(result, 0xff, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
make_array_filled(T const& value, u64 count) noexcept -> Array<T>
{
  auto result = Array<T>{ count };
  for (u64 i = 0; i < count; ++i) {
    std::memcpy(&result[i], &value, sizeof(T));
  }
  return result;
}

template<typename T = byte>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
static auto
borrow_array(void** memory, u64 count) noexcept -> T*
{
  DEBUG_ASSERT_NE(memory, nullptr);
  DEBUG_ASSERT_GE(count, 0);
  if (count > 0) {
    DEBUG_ASSERT_NE(*memory, nullptr);
  }

  DEBUG_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(*memory) % alignof(T), 0);
  DEBUG_ASSERT(is_line_aligned(*memory));

  auto const result    = static_cast<T*>(*memory);
  auto const byte_size = line_align_up(count * sizeof(T));

  KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(result, count * sizeof(T));

  *memory = static_cast<void*>(static_cast<std::byte*>(*memory) + byte_size);
  return result;
}

template<typename T = byte>
static auto
borrow_array_clean(void** memory, u64 count) noexcept -> T*
{
  auto result = borrow_array<T>(memory, count);
  std::memset(result, 0x00, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
borrow_array_filled(void** memory, u64 count) noexcept -> T*
{
  auto result = borrow_array<T>(memory, count);
  std::memset(result, 0xff, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
borrow_array_filled(void** memory, T const& value, u64 count) noexcept -> T*
{
  auto result = borrow_array<T>(memory, count);
  for (u64 i = 0; i < count; ++i) {
    std::memcpy(&result[i], &value, sizeof(T));
  }
  return result;
}
