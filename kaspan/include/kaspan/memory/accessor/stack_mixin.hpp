#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstring>
#include <type_traits>

namespace kaspan {

template<typename Derived, typename T>
  requires(std::is_trivially_copyable_v<T> and std::is_trivially_constructible_v<T> and std::is_trivially_destructible_v<T>)
class stack_mixin
{
public:
  [[nodiscard]] auto operator[](size_t index) -> T&
  {
    return derived()->data()[index];
  }

  [[nodiscard]] auto operator[](size_t index) const -> T const&
  {
    return derived()->data()[index];
  }

  void clear()
  {
    derived()->set_size(0);
  }

  void push(T const& t) noexcept
  {
    auto size = derived()->size();
    KASPAN_VALGRIND_MAKE_MEM_DEFINED(&derived()->data()[size], sizeof(T));
    std::memcpy(&derived()->data()[size], &t, sizeof(T));
    derived()->set_size(size + 1);
  }

  void push(T&& t) noexcept
  {
    auto size = derived()->size();
    KASPAN_VALGRIND_MAKE_MEM_DEFINED(&derived()->data()[size], sizeof(T));
    std::memcpy(&derived()->data()[size], &t, sizeof(T));
    derived()->set_size(size + 1);
  }

  auto back() -> T&
  {
    DEBUG_ASSERT_GT(derived()->size(), 0);
    return derived()->data()[derived()->size() - 1];
  }

  void pop()
  {
    DEBUG_ASSERT_GT(derived()->size(), 0);
    derived()->set_size(derived()->size() - 1);
  }

  [[nodiscard]] auto empty() const -> bool
  {
    return derived()->size() == static_cast<u64>(0);
  }

protected:
  ~stack_mixin() = default;

private:
  auto derived() -> Derived*
  {
    return static_cast<Derived*>(this);
  }

  [[nodiscard]] auto derived() const -> Derived const*
  {
    return static_cast<Derived const*>(this);
  }
};

} // namespace kaspan
