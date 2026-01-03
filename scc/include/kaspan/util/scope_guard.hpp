#pragma once

#include <kaspan/util/pp.hpp>

#include <utility>

namespace kaspan {

template<class fn_t>
class scope_guard final
{
public:
  template<class F0>
  constexpr explicit scope_guard(F0&& f0, fn_t&& fn) noexcept
    : fn(std::forward<fn_t>(fn))
  {
    f0();
  }
  constexpr explicit scope_guard(fn_t&& fn) noexcept
    : fn(std::forward<fn_t>(fn))
  {
  }
  constexpr ~scope_guard() noexcept { fn(); }

  scope_guard(scope_guard const&) = delete;
  scope_guard(scope_guard&&)      = delete;

  auto operator=(scope_guard const&) -> scope_guard& = delete;
  auto operator=(scope_guard&&) -> scope_guard&      = delete;

private:
  fn_t fn;
};

#define SCOPE_GUARD_1(F0) [[maybe_unused]] auto const CAT(guard, __COUNTER__) = kaspan::scope_guard([&] { F0; })
#define SCOPE_GUARD_2(F0, F_) [[maybe_unused]] auto const CAT(guard, __COUNTER__) = kaspan::scope_guard([&] { F0; }, [&] { F_; })
#define SCOPE_GUARD(...) CAT(SCOPE_GUARD_, ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)

} // namespace kaspan
