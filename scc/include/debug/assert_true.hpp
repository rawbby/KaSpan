#pragma once

#include <debug/debug.hpp>
#include <debug/debug_break.hpp>
#include <util/pp.hpp>

#include <format>
#include <print>

template<typename... Args>
void
assert_true(auto&& cond, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (not cond) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

void
assert_true(auto&& cond, std::string_view head)
{
  if (not cond) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT(COND, ...) assert_true(COND, \
"[ASSERTION FAILED]\n"                      \
"  Condition  : " #COND "\n"                \
"  File       : " __FILE__ "\n"             \
"  Line       : " TOSTRING(__LINE__)        \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT(...) IF(KASPAN_DEBUG, ASSERT(__VA_ARGS__))
