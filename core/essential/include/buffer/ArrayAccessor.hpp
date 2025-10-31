#pragma once

#include "debug/Assert.hpp"

#include <util/Arithmetic.hpp>
#include <util/Util.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <type_traits>

template<typename T>
  requires(std::is_trivially_copyable_v<T> and
           std::is_trivially_constructible_v<T> and
           std::is_trivially_destructible_v<T>)
class ArrayAccessor final
{
public:
  using value_type = T;

  /// Non-owning array accessor over a pre-allocated buffer.
  explicit ArrayAccessor(void* __restrict data)
    : data_(data)
  {
    DEBUG_ASSERT_NE(data, nullptr);
    DEBUG_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(data) % alignof(T), 0);
  }

  ArrayAccessor()  = delete;
  ~ArrayAccessor() = default;

  ArrayAccessor(ArrayAccessor const&) noexcept = default;
  ArrayAccessor(ArrayAccessor&&) noexcept      = default;

  auto operator=(ArrayAccessor const&) noexcept -> ArrayAccessor& = default;
  auto operator=(ArrayAccessor&&) noexcept -> ArrayAccessor&      = default;

  static auto borrow(void* __restrict& memory, u64 n) noexcept -> ArrayAccessor
  {
    DEBUG_ASSERT(page_aligned(memory));
    auto const result    = ArrayAccessor{ memory };
    auto const byte_size = page_ceil(n * sizeof(T));
    memory               = static_cast<void*>(static_cast<std::byte*>(memory) + byte_size);
    return result;
  }

  static auto borrow_clean(void* __restrict& memory, u64 n) noexcept -> ArrayAccessor
  {
    auto result = borrow(memory, n);
    result.clear(n);
    return result;
  }

  static auto borrow_filled(void* __restrict& memory, T const& value, u64 n) noexcept -> ArrayAccessor
  {
    auto result = borrow(memory, n);
    result.fill(value, n);
    return result;
  }

  void clear(u64 count)
  {
    std::memset(data_, 0, count * sizeof(T));
  }

  void fill(T const& value, u64 count)
  {
    auto* ptr = data();
    for (u64 i = 0; i < count; ++i) {
      std::memcpy(&ptr[i], &value, sizeof(T));
    }
  }

  [[nodiscard]] auto operator[](u64 index) -> T&
  {
    return data()[index];
  }

  [[nodiscard]] auto data() -> T*
  {
    return static_cast<T*>(data_);
  }

private:
  void* data_;
};
