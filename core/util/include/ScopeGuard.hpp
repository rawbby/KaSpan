#pragma once

#include <utility>

template<class F>
class ScopeGuard final
{
  F f;

public:
  constexpr explicit ScopeGuard(F&& f)
    : f(std::forward<F>(f))
  {
  }

  constexpr ~ScopeGuard() noexcept
  {
    f();
  }

  ScopeGuard(ScopeGuard&& other)           = delete;
  ScopeGuard(const ScopeGuard&)            = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
};

#define SCOPE_GUARD_CAT1(A, B) A##B
#define SCOPE_GUARD_CAT0(A, B) SCOPE_GUARD_CAT1(A, B)

#define SCOPE_GUARD(F)                                               \
  const auto SCOPE_GUARD_CAT0(guard, __COUNTER__) = ScopeGuard([&] { \
    F;                                                               \
  })
