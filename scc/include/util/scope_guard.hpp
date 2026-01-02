#pragma once

#include <util/pp.hpp>

#include <utility>

template<class Fn>
class scope_guard final
{
public:
  template<class F0>
  constexpr explicit scope_guard(F0&& f0, Fn&& fn) noexcept
    : fn(std::forward<Fn>(fn))
  {
    f0();
  }
  constexpr explicit scope_guard(Fn&& fn) noexcept
    : fn(std::forward<Fn>(fn))
  {
  }
  constexpr ~scope_guard() noexcept { fn(); }

  scope_guard(scope_guard const&) = delete;
  scope_guard(scope_guard&&)      = delete;

  auto operator=(scope_guard const&) -> scope_guard& = delete;
  auto operator=(scope_guard&&) -> scope_guard&      = delete;

private:
  Fn fn;
};

#define SCOPE_GUARD_1(F0) [[maybe_unused]] auto const CAT(guard, __COUNTER__) = ScopeGuard([&] { F0; })
#define SCOPE_GUARD_2(F0, F_) [[maybe_unused]] auto const CAT(guard, __COUNTER__) = ScopeGuard([&] { F0; }, [&] { F_; })
#define SCOPE_GUARD(...) CAT(SCOPE_GUARD_, ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)
