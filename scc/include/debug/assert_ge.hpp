#pragma once

#include <debug/debug.hpp>
#include <debug/debug_break.hpp>
#include <util/pp.hpp>

#include <format>
#include <cstdio>
#include <print>

template<typename Lhs, typename Rhs, FormattableConcept... Args>
  requires(not FormattableConcept<Lhs, Rhs>)
void
assert_ge(Lhs&& lhs, Rhs&& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (lhs < rhs) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not FormattableConcept<Lhs, Rhs>)
void
assert_ge(Lhs&& lhs, Rhs&& rhs, std::string_view head)
{
  if (lhs < rhs) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<FormattableConcept Lhs, FormattableConcept Rhs, FormattableConcept... Args>
void
assert_ge(Lhs&& lhs, Rhs&& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (lhs < rhs) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} < {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<FormattableConcept Lhs, FormattableConcept Rhs>
void
assert_ge(Lhs&& lhs, Rhs&& rhs, std::string_view head)
{
  if (lhs < rhs) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} < {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT_GE(LHS, RHS, ...) assert_ge (LHS, RHS, \
"[ASSERTION FAILED]\n"                                \
"  Condition  : " #LHS " >= " #RHS "\n"               \
"  File       : " __FILE__ "\n"                       \
"  Line       : " TOSTRING(__LINE__)                  \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT_GE(...) IF(KASPAN_DEBUG, ASSERT_GE(__VA_ARGS__))
