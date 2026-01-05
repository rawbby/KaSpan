#pragma once

#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/debug_break.hpp>
#include <kaspan/util/formatable.hpp>
#include <kaspan/util/pp.hpp>

#include <cstdio>
#include <format>
#include <print>

namespace kaspan {

template<typename Val, typename Beg, typename End, formattable_concept... Args>
  requires(not formattable_concept<Val, Beg, End>)
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
  requires(not formattable_concept<Val, Beg, End>)
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<formattable_concept Val, formattable_concept Beg, formattable_concept End, formattable_concept... Args>
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {})\n{}", head, val, beg, end, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<formattable_concept Val, formattable_concept Beg, formattable_concept End>
void
assert_in_range(Val&& val, Beg&& beg, End&& end, std::string_view head)
{
  if (val < beg or val >= end) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {})", head, val, beg, end);
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End, formattable_concept... Args>
  requires(not formattable_concept<Val, Beg, End>)
void
assert_in_range_inclusive(Val&& val, Beg&& beg, End&& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (val < beg or val > end) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End>
  requires(not formattable_concept<Val, Beg, End>)
void
assert_in_range_inclusive(Val&& val, Beg&& beg, End&& end, std::string_view head)
{
  if (val < beg or val > end) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<formattable_concept Val, formattable_concept Beg, formattable_concept End, formattable_concept... Args>
void
assert_in_range_inclusive(Val&& val, Beg&& beg, End&& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (val < beg or val > end) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {}]\n{}", head, val, beg, end, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<formattable_concept Val, formattable_concept Beg, formattable_concept End>
void
assert_in_range_inclusive(Val&& val, Beg&& beg, End&& end, std::string_view head)
{
  if (val < beg or val > end) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {}]", head, val, beg, end);
    debug_break();
    std::abort();
  }
}

// clang-format off
#define ASSERT_IN_RANGE(VAL, BEG, END, ...) kaspan::assert_in_range (VAL, BEG, END, \
"[ASSERTION FAILED]\n"                                                      \
"  Condition  : " #VAL " in [" #BEG ", " #END ")\n"                         \
"  File       : " __FILE__ "\n"                                             \
"  Line       : " TOSTRING(__LINE__)                                        \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
#define ASSERT_IN_RANGE_INCLUSIVE(VAL, BEG, END, ...) kaspan::assert_in_range_inclusive (VAL, BEG, END, \
"[ASSERTION FAILED]\n"                                                                \
"  Condition  : " #VAL " in [" #BEG ", " #END "]\n"                                   \
"  File       : " __FILE__ "\n"                                                       \
"  Line       : " TOSTRING(__LINE__)                                                  \
__VA_OPT__(, "  Details    : " __VA_ARGS__))
// clang-format on

#define DEBUG_ASSERT_IN_RANGE(...) IF(KASPAN_DEBUG, ASSERT_IN_RANGE(__VA_ARGS__))
#define DEBUG_ASSERT_IN_RANGE_INCLUSIVE(...) IF(KASPAN_DEBUG, ASSERT_IN_RANGE_INCLUSIVE(__VA_ARGS__))

} // namespace kaspan
