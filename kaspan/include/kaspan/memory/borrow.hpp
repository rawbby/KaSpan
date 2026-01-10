#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class array : public buffer
{
public:
  array() noexcept = default;

  explicit array(u64 count) noexcept(false)
    : buffer(count * sizeof(T))
  {
  }

  array(array const&) = delete;
  array(array&& rhs) noexcept
    : buffer(std::move(rhs))
  {
  }

  auto operator=(array const&) -> array& = delete;
  auto operator=(array&& rhs) noexcept -> array&
  {
    if (this != &rhs) {
      buffer::operator=(std::move(rhs));
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

  auto operator+=(std::ptrdiff_t offset) noexcept -> array&
  {
    data_ = static_cast<void*>(data() + offset);
    return *this;
  }

  auto operator-=(std::ptrdiff_t offset) noexcept -> array&
  {
    data_ = static_cast<void*>(data() - offset);
    return *this;
  }

  auto operator++() noexcept -> array&
  {
    data_ = static_cast<void*>(data() + 1);
    return *this;
  }

  auto operator--() noexcept -> array&
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

  [[nodiscard]] friend auto operator==(array const& lhs, array const& rhs) noexcept -> bool
  {
    return lhs.data() == rhs.data();
  }

  [[nodiscard]] friend auto operator!=(array const& lhs, array const& rhs) noexcept -> bool
  {
    return lhs.data() != rhs.data();
  }

  [[nodiscard]] friend auto operator<(array const& lhs, array const& rhs) noexcept -> bool
  {
    return lhs.data() < rhs.data();
  }

  [[nodiscard]] friend auto operator<=(array const& lhs, array const& rhs) noexcept -> bool
  {
    return lhs.data() <= rhs.data();
  }

  [[nodiscard]] friend auto operator>(array const& lhs, array const& rhs) noexcept -> bool
  {
    return lhs.data() > rhs.data();
  }

  [[nodiscard]] friend auto operator>=(array const& lhs, array const& rhs) noexcept -> bool
  {
    return lhs.data() >= rhs.data();
  }

  [[nodiscard]] friend auto operator==(array const& arr, std::nullptr_t) noexcept -> bool
  {
    return arr.data() == nullptr;
  }

  [[nodiscard]] friend auto operator==(std::nullptr_t, array const& arr) noexcept -> bool
  {
    return arr.data() == nullptr;
  }

  [[nodiscard]] friend auto operator!=(array const& arr, std::nullptr_t) noexcept -> bool
  {
    return arr.data() != nullptr;
  }

  [[nodiscard]] friend auto operator!=(std::nullptr_t, array const& arr) noexcept -> bool
  {
    return arr.data() != nullptr;
  }

  [[nodiscard]] friend auto operator+(std::ptrdiff_t offset, array const& arr) noexcept -> T*
  {
    return arr.data() + offset;
  }

  [[nodiscard]] explicit operator T*() const noexcept
  {
    return data();
  }

  [[nodiscard]] explicit operator T const*() const noexcept
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
make_array(u64 count) noexcept -> array<T>
{
  auto result = array<T>{ count };
  return result;
}

template<typename T = byte>
static auto
make_array_clean(u64 count) noexcept -> array<T>
{
  auto result = array<T>{ count };
  std::memset(result, 0x00, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
make_array_filled(u64 count) noexcept -> array<T>
{
  auto result = array<T>{ count };
  std::memset(result, 0xff, count * sizeof(T));
  return result;
}

template<typename T = byte>
static auto
make_array_filled(T const& value, u64 count) noexcept -> array<T>
{
  auto result = array<T>{ count };
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
  DEBUG_ASSERT_NE(*memory, nullptr);
  DEBUG_ASSERT_EQ(linesize() % alignof(T), 0);
  DEBUG_ASSERT(is_line_aligned(*memory));
  auto const result    = static_cast<T*>(*memory);
  auto const byte_size = line_align_up(count * sizeof(T));
  KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(*memory, byte_size);
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

} // namespace kaspan
