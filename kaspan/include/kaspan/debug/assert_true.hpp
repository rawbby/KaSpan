#pragma once

#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/debug_break.hpp>
#include <kaspan/util/formatable.hpp> // formattable_concept
#include <kaspan/util/pp.hpp>

#include <cstdio>
#include <format>
#include <print>

namespace kaspan {

template<formattable_concept... Args>
void
assert_true(auto const& cond, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (not cond) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

void
assert_true(auto const& cond, std::string_view head)
{
  if (not cond) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT(COND, ...) kaspan::assert_true(COND, \
"[ASSERTION FAILED]\n"                      \
"  Condition  : " #COND "\n"                \
"  File       : " __FILE__ "\n"             \
"  Line       : " TOSTRING(__LINE__)        \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT(...) IF(KASPAN_DEBUG, ASSERT(__VA_ARGS__))

} // namespace kaspan
