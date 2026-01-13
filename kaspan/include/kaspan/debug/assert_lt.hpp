#pragma once

#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/debug_break.hpp>
#include <kaspan/util/comparable.hpp>
#include <kaspan/util/formatable.hpp> // formattable_concept
#include <kaspan/util/pp.hpp>
#include <kaspan/util/cmp.hpp>

#include <cstdio>
#include <format>
#include <print>

namespace kaspan {

template<typename Lhs, typename Rhs, formattable_concept... Args>
  requires(not formattable_concept<Lhs, Rhs>)
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  bool const cond = [&] {
    if constexpr (comparable_concept<Lhs, Rhs>) {
      return cmp_greater_equal(lhs, rhs);
    } else {
      return lhs >= rhs;
    }
  }();

  if (cond) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not formattable_concept<Lhs, Rhs>)
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  bool const cond = [&] {
    if constexpr (comparable_concept<Lhs, Rhs>) {
      return cmp_greater_equal(lhs, rhs);
    } else {
      return lhs >= rhs;
    }
  }();

  if (cond) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<formattable_concept Lhs, formattable_concept Rhs, formattable_concept... Args>
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  bool const cond = [&] {
    if constexpr (comparable_concept<Lhs, Rhs>) {
      return cmp_greater_equal(lhs, rhs);
    } else {
      return lhs >= rhs;
    }
  }();

  if (cond) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} >= {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<formattable_concept Lhs, formattable_concept Rhs>
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  bool const cond = [&] {
    if constexpr (comparable_concept<Lhs, Rhs>) {
      return cmp_greater_equal(lhs, rhs);
    } else {
      return lhs >= rhs;
    }
  }();

  if (cond) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} >= {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT_LT(LHS, RHS, ...) kaspan::assert_lt (LHS, RHS, \
"[ASSERTION FAILED]\n"                                \
"  Condition  : " #LHS " < " #RHS "\n"                \
"  File       : " __FILE__ "\n"                       \
"  Line       : " TOSTRING(__LINE__)                  \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT_LT(...) IF(KASPAN_DEBUG, ASSERT_LT(__VA_ARGS__))

} // namespace kaspan
