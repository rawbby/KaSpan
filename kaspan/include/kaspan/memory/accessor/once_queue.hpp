#pragma once

#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/accessor/once_queue_accessor.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstring>
#include <type_traits>
#include <utility>

namespace kaspan {

template<typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class once_queue final : public buffer
{
public:
  once_queue() noexcept = default;
  ~once_queue()         = default;

  template<ArithmeticConcept Size>
  explicit once_queue(Size size) noexcept(false)
    : buffer(static_cast<u64>(size) * sizeof(T))
  {
    DEBUG_ASSERT_GE(size, 0);
    DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
    IF(KASPAN_DEBUG, size_ = static_cast<u64>(size));
  }

  once_queue(once_queue const&) = delete;
  once_queue(once_queue&& rhs) noexcept
    : buffer(std::move(rhs))
    , beg_(rhs.beg_)
    , end_(rhs.end_)
  {
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0);
    rhs.beg_ = 0;
    rhs.end_ = 0;
  }

  auto operator=(once_queue const&) -> once_queue& = delete;
  auto operator=(once_queue&& rhs) noexcept -> once_queue&
  {
    buffer::operator=(std::move(rhs));
    beg_ = rhs.beg_;
    end_ = rhs.end_;
    IF(KASPAN_DEBUG, size_ = rhs.size_);
    IF(KASPAN_DEBUG, rhs.size_ = 0;)
    rhs.beg_ = 0;
    rhs.end_ = 0;
    return *this;
  }

  [[nodiscard]] auto data() -> T*
  {
    return static_cast<T*>(buffer::data());
  }

  [[nodiscard]] auto data() const -> T const*
  {
    return static_cast<T const*>(buffer::data());
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
  u64 beg_ = 0;
  u64 end_ = 0;
  IF(KASPAN_DEBUG, u64 size_ = 0);
};

template<typename T, ArithmeticConcept Size>
auto
make_once_queue(Size size) -> once_queue<T>
{
  return once_queue<T>{ size };
}

} // namespace kaspan
