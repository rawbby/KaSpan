#pragma once

#include <debug/debug.hpp>
#include <debug/debug_break.hpp>
#include <util/pp.hpp>

#include <cstdio>
#include <format>
#include <print>

template<FormattableConcept... Args>
void
assert_false(auto&& cond, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (cond) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

void
assert_false(auto cond, std::string_view head)
{
  if (cond) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT_NOT(COND, ...) assert_false(COND, \
"[ASSERTION FAILED]\n"                           \
"  Condition  : not(" #COND ")\n"                \
"  File       : " __FILE__ "\n"                  \
"  Line       : " TOSTRING(__LINE__)             \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT_NOT(...) IF(KASPAN_DEBUG, ASSERT_NOT(__VA_ARGS__))
