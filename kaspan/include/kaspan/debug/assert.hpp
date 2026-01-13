#pragma once

#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/debug_break.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/pp.hpp>

#include <cstdio>
#include <format>
#include <print>
#include <string_view>
#include <utility>

namespace kaspan {

// clang-format off

#define ASSERT(COND, ...)                               \
    kaspan::assert_true(COND,                           \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #COND "\n"                        \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_NE(LHS, RHS, ...)                        \
    kaspan::assert_ne(LHS, RHS,                         \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #LHS " != " #RHS "\n"             \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_LT(LHS, RHS, ...)                        \
    kaspan::assert_lt(LHS, RHS,                         \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #LHS " < " #RHS "\n"              \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_LE(LHS, RHS, ...)                        \
    kaspan::assert_le(LHS, RHS,                         \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #LHS " <= " #RHS "\n"             \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_GT(LHS, RHS, ...)                        \
    kaspan::assert_gt(LHS, RHS,                         \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #LHS " > " #RHS "\n"              \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_GE(LHS, RHS, ...)                        \
    kaspan::assert_ge(LHS, RHS,                         \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #LHS " >= " #RHS "\n"             \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_NOT(COND, ...)                           \
    kaspan::assert_false(COND,                          \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : not(" #COND ")\n"                   \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_EQ(LHS, RHS, ...)                        \
    kaspan::assert_eq(LHS, RHS,                         \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #LHS " == " #RHS "\n"             \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
__VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_IN_RANGE(VAL, BEG, END, ...)             \
    kaspan::assert_in_range(VAL, BEG, END,              \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #VAL " in [" #BEG ", " #END ")\n" \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define ASSERT_IN_RANGE_INCLUSIVE(VAL, BEG, END, ...)   \
    kaspan::assert_in_range_inclusive(VAL, BEG, END,    \
    "[ASSERTION FAILED]\n"                              \
    "  Condition  : " #VAL " in [" #BEG ", " #END "]\n" \
    "  File       : " __FILE__ "\n"                     \
    "  Line       : " TOSTRING(__LINE__)                \
    __VA_OPT__(, "  Details    : " __VA_ARGS__))

#define DEBUG_ASSERT(...)     IF(KASPAN_DEBUG, ASSERT(__VA_ARGS__))
#define DEBUG_ASSERT_NE(...)  IF(KASPAN_DEBUG, ASSERT_NE(__VA_ARGS__))
#define DEBUG_ASSERT_LT(...)  IF(KASPAN_DEBUG, ASSERT_LT(__VA_ARGS__))
#define DEBUG_ASSERT_LE(...)  IF(KASPAN_DEBUG, ASSERT_LE(__VA_ARGS__))
#define DEBUG_ASSERT_GT(...)  IF(KASPAN_DEBUG, ASSERT_GT(__VA_ARGS__))
#define DEBUG_ASSERT_GE(...)  IF(KASPAN_DEBUG, ASSERT_GE(__VA_ARGS__))
#define DEBUG_ASSERT_NOT(...) IF(KASPAN_DEBUG, ASSERT_NOT(__VA_ARGS__))
#define DEBUG_ASSERT_EQ(...)  IF(KASPAN_DEBUG, ASSERT_EQ(__VA_ARGS__))

#define DEBUG_ASSERT_IN_RANGE(...)           IF(KASPAN_DEBUG, ASSERT_IN_RANGE(__VA_ARGS__))
#define DEBUG_ASSERT_IN_RANGE_INCLUSIVE(...) IF(KASPAN_DEBUG, ASSERT_IN_RANGE_INCLUSIVE(__VA_ARGS__))

// clang-format on

namespace internal {

template<typename T>
concept formattable_concept = requires { std::formatter<std::remove_cvref_t<T>>{}; };

template<typename Lhs, typename Rhs>
  requires(not arithmetic_concept<Lhs> or not arithmetic_concept<Rhs>)
constexpr bool
eq(Lhs const& lhs, Rhs const& rhs) noexcept
{
  return lhs == rhs;
}

template<typename Lhs, typename Rhs>
  requires(not arithmetic_concept<Lhs> or not arithmetic_concept<Rhs>)
constexpr bool
lt(Lhs const& lhs, Rhs const& rhs) noexcept
{
  return lhs < rhs;
}

template<arithmetic_concept Lhs, arithmetic_concept Rhs>
constexpr bool
eq(Lhs lhs, Rhs rhs) noexcept
{
  if constexpr (signed_concept<Lhs> == signed_concept<Rhs>)
    return lhs == rhs;
  else if constexpr (signed_concept<Lhs>)
    return lhs >= 0 && std::make_unsigned_t<Lhs>(lhs) == rhs;
  else
    return rhs >= 0 && std::make_unsigned_t<Rhs>(rhs) == lhs;
}

template<arithmetic_concept Lhs, arithmetic_concept Rhs>
constexpr bool
lt(Lhs lhs, Rhs rhs) noexcept
{
  if constexpr (signed_concept<Lhs> == signed_concept<Rhs>)
    return lhs < rhs;
  else if constexpr (signed_concept<Lhs>)
    return lhs < 0 || std::make_unsigned_t<Lhs>(lhs) < rhs;
  else
    return rhs >= 0 && lhs < std::make_unsigned_t<Rhs>(rhs);
}

constexpr bool
ne(auto const& lhs, auto const& rhs) noexcept
{
  return !eq(lhs, rhs);
}

constexpr bool
gt(auto const& lhs, auto const& rhs) noexcept
{
  return lt(rhs, lhs);
}

constexpr bool
le(auto const& lhs, auto const& rhs) noexcept
{
  return !gt(lhs, rhs);
}

constexpr bool
ge(auto const& lhs, auto const& rhs) noexcept
{
  return !lt(lhs, rhs);
}

constexpr bool
in_range(auto const& val, auto const& beg, auto const& end) noexcept
{
  return ge(val, beg) & lt(val, end);
}

constexpr bool
in_range_inclusive(auto const& val, auto const& beg, auto const& end) noexcept
{
  return ge(val, beg) & le(val, end);
}

}

template<internal::formattable_concept... Args>
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

template<internal::formattable_concept... Args>
void
assert_false(auto const& cond, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
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

template<typename Lhs, typename Rhs, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_eq(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::ne(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_eq(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::ne(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs, internal::formattable_concept... Args>
void
assert_eq(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::ne(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} != {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs>
void
assert_eq(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::ne(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} != {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_ne(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::eq(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_ne(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::eq(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs, internal::formattable_concept... Args>
void
assert_ne(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::eq(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} == {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs>
void
assert_ne(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::eq(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} == {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::ge(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::ge(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs, internal::formattable_concept... Args>
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::ge(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} >= {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs>
void
assert_lt(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::ge(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} >= {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_gt(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::le(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_gt(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::le(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs, internal::formattable_concept... Args>
void
assert_gt(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::le(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} <= {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs>
void
assert_gt(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::le(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} <= {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_le(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::gt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_le(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::gt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs, internal::formattable_concept... Args>
void
assert_le(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::gt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} > {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs>
void
assert_le(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::gt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} > {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_ge(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::lt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Lhs, typename Rhs>
  requires(not internal::formattable_concept<Lhs> or not internal::formattable_concept<Rhs>)
void
assert_ge(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::lt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs, internal::formattable_concept... Args>
void
assert_ge(Lhs const& lhs, Rhs const& rhs, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (internal::lt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} < {}\n{}", head, lhs, rhs, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Lhs, internal::formattable_concept Rhs>
void
assert_ge(Lhs const& lhs, Rhs const& rhs, std::string_view head)
{
  if (internal::lt(lhs, rhs)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} < {}", head, lhs, rhs);
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Val> or not internal::formattable_concept<Beg> or not internal::formattable_concept<End>)
void
assert_in_range(Val const& val, Beg const& beg, End const& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End>
  requires(not internal::formattable_concept<Val> or not internal::formattable_concept<Beg> or not internal::formattable_concept<End>)
void
assert_in_range(Val const& val, Beg const& beg, End const& end, std::string_view head)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Val, internal::formattable_concept Beg, internal::formattable_concept End, internal::formattable_concept... Args>
void
assert_in_range(Val const& val, Beg const& beg, End const& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {})\n{}", head, val, beg, end, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Val, internal::formattable_concept Beg, internal::formattable_concept End>
void
assert_in_range(Val const& val, Beg const& beg, End const& end, std::string_view head)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {})", head, val, beg, end);
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End, internal::formattable_concept... Args>
  requires(not internal::formattable_concept<Val> or not internal::formattable_concept<Beg> or not internal::formattable_concept<End>)
void
assert_in_range_inclusive(Val const& val, Beg const& beg, End const& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (!internal::in_range_inclusive(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}\n{}", head, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<typename Val, typename Beg, typename End>
  requires(not internal::formattable_concept<Val> or not internal::formattable_concept<Beg> or not internal::formattable_concept<End>)
void
assert_in_range_inclusive(Val const& val, Beg const& beg, End const& end, std::string_view head)
{
  if (!internal::in_range_inclusive(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}", head);
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Val, internal::formattable_concept Beg, internal::formattable_concept End, internal::formattable_concept... Args>
void
assert_in_range_inclusive(Val const& val, Beg const& beg, End const& end, std::string_view head, std::format_string<Args...> fmt, Args&&... args)
{
  if (!internal::in_range_inclusive(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {}]\n{}", head, val, beg, end, std::format(fmt, std::forward<Args>(args)...));
    debug_break();
    std::abort();
  }
}

template<internal::formattable_concept Val, internal::formattable_concept Beg, internal::formattable_concept End>
void
assert_in_range_inclusive(Val const& val, Beg const& beg, End const& end, std::string_view head)
{
  if (!internal::in_range_inclusive(val, beg, end)) [[unlikely]] {
    std::println(stderr, "{}\n  Evaluation : {} in [{}, {}]", head, val, beg, end);
    debug_break();
    std::abort();
  }
}

} // namespace kaspan
