#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <type_traits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
class once_queue_accessor final
{
public:
  once_queue_accessor()  = default;
  ~once_queue_accessor() = default;

  template<arithmetic_concept Size>
  explicit once_queue_accessor(
    void* data,
    Size  size)
    : data_(size == 0 ? nullptr : data)
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    DEBUG_ASSERT((size == 0 && data_ == nullptr) || (size > 0 && data_ != nullptr));
    IF(KASPAN_DEBUG, size_ = integral_cast<u64>(size));
    KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(data, size * sizeof(T));
  }

  once_queue_accessor(once_queue_accessor const& rhs) noexcept = default;
  once_queue_accessor(once_queue_accessor&& rhs) noexcept      = default;

  auto operator=(once_queue_accessor const& rhs) noexcept -> once_queue_accessor& = default;
  auto operator=(once_queue_accessor&& rhs) noexcept -> once_queue_accessor&      = default;

  auto data() -> T*
  {
    return static_cast<T*>(data_);
  }

  [[nodiscard]] auto data() const -> T const*
  {
    return static_cast<T const*>(data_);
  }

  [[nodiscard]] auto size() const -> u64
  {
    return end_ - beg_;
  }

  [[nodiscard]] auto empty() const -> bool
  {
    return end_ == beg_;
  }

  auto begin() const
  {
    return data() + beg_;
  }

  auto end() const
  {
    return data() + end_;
  }

  auto begin()
  {
    return data() + beg_;
  }

  auto end()
  {
    return data() + end_;
  }

  void clear()
  {
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(data() + beg_, size() * sizeof(T));
    beg_ = 0;
    end_ = 0;
  }

  void push_back(
    T t) noexcept
  {
    data()[end_++] = t;
  }

  auto pop_front() noexcept -> T
  {
    DEBUG_ASSERT_LT(beg_, end_);
    T item = data()[beg_];
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(data() + beg_, sizeof(T));
    ++beg_;
    return item;
  }

private:
  void* data_ = nullptr;
  u64   beg_  = 0;
  u64   end_  = 0;
  IF(KASPAN_DEBUG,
     u64 size_ = 0);
};

template<typename T,
         arithmetic_concept Size>
auto
borrow_once_queue(
  void** memory,
  Size   size) -> once_queue_accessor<T>
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = integral_cast<u64>(size);
  return once_queue_accessor<T>{ borrow_array<T>(memory, size64), size64 };
}

template<typename T,
         arithmetic_concept Size>
auto
view_once_queue(
  void* data,
  Size  size) -> once_queue_accessor<T>
{
  return once_queue_accessor<T>{ data, size };
}

} // namespace kaspan
