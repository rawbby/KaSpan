#pragma once

#include <utility>

namespace ka_span {

template<class F>
class scope_guard
{
  F f;

public:
  // clang-format off
  constexpr explicit scope_guard(F&& f) : f(std::forward<F>(f)) {}
  constexpr ~scope_guard() noexcept { f(); }
  // clang-format on

  scope_guard(scope_guard&& other)           = delete;
  scope_guard(const scope_guard&)            = delete;
  scope_guard& operator=(const scope_guard&) = delete;
};

#define SCOPE_GUARD_CAT1(A, B) A##B
#define SCOPE_GUARD_CAT0(A, B) SCOPE_GUARD_CAT1(A, B)

#define SCOPE_GUARD(F)                                                           \
  const auto SCOPE_GUARD_CAT0(guard, __COUNTER__) = ::ka_span::scope_guard([&] { \
    F;                                                                           \
  })

} // namespace ka_span
