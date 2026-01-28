#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/pp.hpp>

#include <cstddef>
#include <cstring>
#include <limits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>)
class array : public buffer
{
public:
  array() noexcept = default;

  explicit array(arithmetic_concept auto count) noexcept(false)
    : buffer(integral_cast<u64>(count) * sizeof(T))
  {
    DEBUG_ASSERT_GE(count, 0);
    DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  }

  array(array const&) = delete;
  array(
    array&& rhs) noexcept
    : buffer(std::move(rhs))
  {
  }

  auto operator=(array const&) -> array& = delete;
  auto operator=(
    array&& rhs) noexcept -> array&
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

  template<arithmetic_concept Index>
  [[nodiscard]] auto operator[](
    Index idx) const noexcept -> T&
  {
    DEBUG_ASSERT_GE(idx, 0);
    DEBUG_ASSERT_LE(idx, std::numeric_limits<u64>::max());
    return data()[integral_cast<u64>(idx)];
  }

  [[nodiscard]] auto operator+(
    std::ptrdiff_t offset) const noexcept -> T*
  {
    return data() + offset;
  }

  [[nodiscard]] auto operator-(
    std::ptrdiff_t offset) const noexcept -> T*
  {
    return data() - offset;
  }

  auto operator+=(
    std::ptrdiff_t offset) noexcept -> array&
  {
    data_ = static_cast<void*>(data() + offset);
    return *this;
  }

  auto operator-=(
    std::ptrdiff_t offset) noexcept -> array&
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

  auto operator++(
    int) noexcept -> T*
  {
    T* tmp = data();
    data_  = static_cast<void*>(data() + 1);
    return tmp;
  }

  auto operator--(
    int) noexcept -> T*
  {
    T* tmp = data();
    data_  = static_cast<void*>(data() - 1);
    return tmp;
  }

  [[nodiscard]] friend auto operator==(
    array const& lhs,
    array const& rhs) noexcept -> bool
  {
    return lhs.data() == rhs.data();
  }

  [[nodiscard]] friend auto operator!=(
    array const& lhs,
    array const& rhs) noexcept -> bool
  {
    return lhs.data() != rhs.data();
  }

  [[nodiscard]] friend auto operator<(
    array const& lhs,
    array const& rhs) noexcept -> bool
  {
    return lhs.data() < rhs.data();
  }

  [[nodiscard]] friend auto operator<=(
    array const& lhs,
    array const& rhs) noexcept -> bool
  {
    return lhs.data() <= rhs.data();
  }

  [[nodiscard]] friend auto operator>(
    array const& lhs,
    array const& rhs) noexcept -> bool
  {
    return lhs.data() > rhs.data();
  }

  [[nodiscard]] friend auto operator>=(
    array const& lhs,
    array const& rhs) noexcept -> bool
  {
    return lhs.data() >= rhs.data();
  }

  [[nodiscard]] friend auto operator==(
    array const& arr,
    std::nullptr_t) noexcept -> bool
  {
    return arr.data() == nullptr;
  }

  [[nodiscard]] friend auto operator==(
    std::nullptr_t,
    array const& arr) noexcept -> bool
  {
    return arr.data() == nullptr;
  }

  [[nodiscard]] friend auto operator!=(
    array const& arr,
    std::nullptr_t) noexcept -> bool
  {
    return arr.data() != nullptr;
  }

  [[nodiscard]] friend auto operator!=(
    std::nullptr_t,
    array const& arr) noexcept -> bool
  {
    return arr.data() != nullptr;
  }

  [[nodiscard]] friend auto operator+(
    std::ptrdiff_t offset,
    array const&   arr) noexcept -> T*
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

  void fill(
    std::byte               value,
    arithmetic_concept auto size)
  {
    std::memset(data(), static_cast<int>(value), integral_cast<std::size_t>(size) * sizeof(T));
  }

  void fill(
    arithmetic_concept auto size)
  {
    fill(static_cast<std::byte>(0xff), size);
  }

  void clear(
    arithmetic_concept auto size)
  {
    fill(static_cast<std::byte>(0x00), size);
  }
};

template<typename T = byte,
         arithmetic_concept Count>
static auto
make_array(
  Count count) noexcept -> array<T>
{
  auto result = array<T>{ count };
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
static auto
make_array_clean(
  Count count) noexcept -> array<T>
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto result = array<T>{ count };
  result.clear(count);
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
static auto
make_array_filled(
  Count count) noexcept -> array<T>
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  auto       result  = array<T>{ count64 };
  std::memset(result.data(), 0xff, count64 * sizeof(T));
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
static auto
make_array_filled(
  T const& value,
  Count    count) noexcept -> array<T>
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  auto       result  = array<T>{ count64 };
  for (u64 i = 0; i < count64; ++i) {
    std::memcpy(&result[i], &value, sizeof(T));
  }
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>)
static auto
borrow_array(
  void** memory,
  Count  count) noexcept -> T*
{
  DEBUG_ASSERT_NE(memory, nullptr);
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  if (count64 == 0) {
    return nullptr;
  }
  DEBUG_ASSERT_NE(*memory, nullptr);
  DEBUG_ASSERT_EQ(linesize() % alignof(T), 0);
  DEBUG_ASSERT(is_line_aligned(*memory));
  auto const result    = static_cast<T*>(*memory);
  auto const byte_size = line_align_up(count64 * sizeof(T));
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(*memory, byte_size);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(*memory, byte_size);
  *memory = static_cast<void*>(static_cast<std::byte*>(*memory) + byte_size);
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
static auto
borrow_array_clean(
  void** memory,
  Count  count) noexcept -> T*
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  auto       result  = borrow_array<T>(memory, count64);
  std::memset(result, 0x00, count64 * sizeof(T));
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
static auto
borrow_array_filled(
  void** memory,
  Count  count) noexcept -> T*
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  auto       result  = borrow_array<T>(memory, count64);
  std::memset(result, 0xff, count64 * sizeof(T));
  return result;
}

template<typename T = byte,
         arithmetic_concept Count>
static auto
borrow_array_filled(
  void**   memory,
  T const& value,
  Count    count) noexcept -> T*
{
  DEBUG_ASSERT_GE(count, 0);
  DEBUG_ASSERT_LE(count, std::numeric_limits<u64>::max());
  auto const count64 = integral_cast<u64>(count);
  auto       result  = borrow_array<T>(memory, count64);
  for (u64 i = 0; i < count64; ++i) {
    std::memcpy(&result[i], &value, sizeof(T));
  }
  return result;
}

} // namespace kaspan
