#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstdlib>
#include <type_traits>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class once_queue_accessor final
{
public:
  once_queue_accessor()  = default;
  ~once_queue_accessor() = default;

  explicit once_queue_accessor(void* data, u64 size)
    : data_(data)
  {
    IF(KASPAN_DEBUG, size_ = size);
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

  void clear()
  {
    KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(data() + beg_, size() * sizeof(T));
    beg_ = 0;
    end_ = 0;
  }

  void push_back(T t) noexcept
  {
    data()[end_++] = t;
  }

  void pop_front() noexcept
  {
    DEBUG_ASSERT_LT(beg_, end_);
    T item = data()[beg_];
    KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(data() + beg_, sizeof(T));
    ++beg_;
  }

private:
  void* data_ = nullptr;
  u64   beg_  = 0;
  u64   end_  = 0;
  IF(KASPAN_DEBUG, u64 size_ = 0);
};

template<typename T>
auto
borrow_once_queue(void** memory, u64 size) -> once_queue_accessor<T>
{
  return once_queue_accessor<T>{ borrow_array<T>(memory, size), size };
}

template<typename T>
auto
view_once_queue(void* data, u64 size) -> once_queue_accessor<T>
{
  return once_queue_accessor<T>{ data, size };
}

} // namespace kaspan
