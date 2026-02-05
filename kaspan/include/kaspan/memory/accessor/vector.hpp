#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
class vector
{
public:
  constexpr vector() noexcept = default;

  explicit vector(
    integral_c auto initial_capacity)
    : capacity_(integral_cast<u64>(initial_capacity))
    , data_(line_alloc<T>(capacity_))
  {
  }

  ~vector()
  {
    line_free(data_);
  }

  vector(vector const&) = delete;
  vector(
    vector&& rhs) noexcept
    : capacity_(rhs.capacity_)
    , size_(rhs.size_)
    , data_(rhs.data_)
  {
    rhs.capacity_ = 0;
    rhs.size_     = 0;
    rhs.data_     = nullptr;
  }

  auto operator=(vector const&) -> vector& = delete;
  auto operator=(
    vector&& rhs) noexcept -> vector&
  {
    if (this != &rhs) {
      line_free(data_);
      capacity_     = rhs.capacity_;
      size_         = rhs.size_;
      data_         = rhs.data_;
      rhs.capacity_ = 0;
      rhs.size_     = 0;
      rhs.data_     = nullptr;
    }
    return *this;
  }

  void push_back(
    T value)
  {
    if (size_ >= capacity_) [[unlikely]] {
      grow();
    }
    data_[size_++] = value;
  }

  /// context must ensure no overflow
  void unsafe_push_back(
    T value)
  {
    DEBUG_ASSERT_LT(size_, capacity_);
    data_[size_++] = value;
  }

  void reserve(
    u64 new_capacity)
  {
    if (new_capacity > capacity_) [[likely]] {
      auto* new_data = line_alloc<T>(new_capacity);
      std::memcpy(new_data, data_, size_ * sizeof(T));
      line_free(data_);
      data_     = new_data;
      capacity_ = new_capacity;
    }
  }

  void resize(
    u64 new_size)
  {
    if (new_size > capacity_) {
      reserve(new_size);
    }
    size_ = new_size;
  }

  void clear()
  {
    size_ = 0;
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(data_, capacity_ * sizeof(T));
  }

  [[nodiscard]] auto data() noexcept -> T*
  {
    return data_;
  }

  [[nodiscard]] auto data() const noexcept -> T const*
  {
    return data_;
  }

  [[nodiscard]] auto size() const noexcept -> u64
  {
    return size_;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return size_ == 0;
  }

  auto operator[](
    u64 i) -> T&
  {
    DEBUG_ASSERT_LT(i, size_);
    return data_[i];
  }

  auto operator[](
    u64 i) const -> T const&
  {
    DEBUG_ASSERT_LT(i, size_);
    return data_[i];
  }

  auto back() -> T&
  {
    DEBUG_ASSERT_GT(size_, 0);
    return data_[size_ - 1];
  }

  auto back() const -> T const&
  {
    DEBUG_ASSERT_GT(size_, 0);
    return data_[size_ - 1];
  }

  void pop_back()
  {
    DEBUG_ASSERT_GT(size_, 0);
    --size_;
  }

  auto begin() noexcept -> T*
  {
    return data_;
  }

  auto end() noexcept -> T*
  {
    return data_ + size_;
  }

  auto begin() const noexcept -> T const*
  {
    return data_;
  }

  auto end() const noexcept -> T const*
  {
    return data_ + size_;
  }

  auto cbegin() const noexcept -> T const*
  {
    return data_;
  }

  auto cend() const noexcept -> T const*
  {
    return data_ + size_;
  }

private:
  void grow()
  {
    if (capacity_ == 0) [[unlikely]] {
      // minimum capacity of 32 Kibi Bytes
      reserve(32 * 1024);
    } else {
      // doubling strategy
      reserve(capacity_ << 1);
    }
  }

  u64 capacity_ = 0;
  u64 size_     = 0;
  T*  data_     = nullptr;
};

} // namespace kaspan
