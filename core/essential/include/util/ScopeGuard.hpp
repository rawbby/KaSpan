#pragma once

#include <pp/PP.hpp>
#include <utility>

template<class Fn>
class ScopeGuard final
{
public:
  template<class F0>
  constexpr explicit ScopeGuard(F0&& f0, Fn&& fn) noexcept
    : fn(std::forward<Fn>(fn))
  {
    f0();
  }
  constexpr explicit ScopeGuard(Fn&& fn) noexcept
    : fn(std::forward<Fn>(fn))
  {
  }
  constexpr ~ScopeGuard() noexcept
  {
    fn();
  }

  ScopeGuard(ScopeGuard const&) = delete;
  ScopeGuard(ScopeGuard&&)      = delete;

  auto operator=(ScopeGuard const&) -> ScopeGuard& = delete;
  auto operator=(ScopeGuard&&) -> ScopeGuard&      = delete;

private:
  Fn fn;
};

#define SCOPE_GUARD_1(F0)     [[maybe_unused]] auto const CAT(guard, __COUNTER__) = ScopeGuard([&]{F0;})
#define SCOPE_GUARD_2(F0, F_) [[maybe_unused]] auto const CAT(guard, __COUNTER__) = ScopeGuard([&]{F0;}, [&]{F_;})
#define SCOPE_GUARD(...) CAT(SCOPE_GUARD_, ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)
