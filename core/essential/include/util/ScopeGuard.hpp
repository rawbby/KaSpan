#pragma once

#include <pp/PP.hpp>
#include <utility>

template<class F>
class ScopeGuard final
{
public:
  constexpr explicit ScopeGuard(F&& f) noexcept
    : f_(std::forward<F>(f))
  {
  }
  constexpr ~ScopeGuard() noexcept
  {
    f_();
  }

  ScopeGuard(ScopeGuard const&) = delete;
  ScopeGuard(ScopeGuard&&)      = delete;

  auto operator=(ScopeGuard const&) -> ScopeGuard& = delete;
  auto operator=(ScopeGuard&&) -> ScopeGuard&      = delete;

private:
  F f_;
};

#define SCOPE_GUARD(F)                                  \
  const auto CAT(guard, __COUNTER__) = ScopeGuard([&] { \
    F;                                                  \
  })
