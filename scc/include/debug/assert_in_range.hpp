#pragma once

#include <debug/debug.hpp>
#include <debug/debug_break.hpp>
#include <util/formatable.hpp>
#include <util/pp.hpp>

#include <format>
#include <print>

template<typename Val, typename Beg, typename End, FormattableConcept... Args>
  requires(not FormattableConcept<Val, Beg, End>)
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End>
  requires(not FormattableConcept<Val, Beg, End>)
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<FormattableConcept Val, FormattableConcept Beg, FormattableConcept End, FormattableConcept... Args>
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {})\n{}", head, val, beg, end, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<FormattableConcept Val, FormattableConcept Beg, FormattableConcept End>
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {})", head, val, beg, end);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT_IN_RANGE(VAL, BEG, END, ...) assert_in_range (VAL, BEG, END, \
"[ASSERTION FAILED]\n"                                                      \
"  Condition  : " #VAL " in [" #BEG ", " #END ")\n"                         \
"  File       : " __FILE__ "\n"                                             \
"  Line       : " TOSTRING(__LINE__)                                        \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT_IN_RANGE(...) IF(KASPAN_DEBUG, ASSERT_IN_RANGE(__VA_ARGS__))
