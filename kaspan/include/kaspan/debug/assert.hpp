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

#define ASSERT_NULLPTR(PTR, ...) \
  ASSERT_EQ(PTR, nullptr __VA_OPT__(, __VA_ARGS__))

#define ASSERT_POINTER(PTR, ...) \
  ASSERT_NE(PTR, nullptr __VA_OPT__(, __VA_ARGS__))

#define ASSERT_SIZE_POINTER(SIZE, PTR, ...)      \
  if (internal::eq(SIZE, 0)) ASSERT_NULLPTR(PTR); \
  else ASSERT_POINTER(PTR)

#define ASSERT_IN_RANGE(VAL, BEG, END, ...)      \
  ASSERT_GE(VAL, BEG __VA_OPT__(, __VA_ARGS__)); \
  ASSERT_LT(VAL, END __VA_OPT__(, __VA_ARGS__))

#define ASSERT_IN_RANGE_INCLUSIVE(VAL, BEG, END, ...) \
  ASSERT_GE(VAL, BEG __VA_OPT__(, __VA_ARGS__));      \
  ASSERT_LE(VAL, END __VA_OPT__(, __VA_ARGS__))

#define DEBUG_ASSERT(...)                    IF(KASPAN_DEBUG, ASSERT(__VA_ARGS__),                    ((void)0))
#define DEBUG_ASSERT_NE(...)                 IF(KASPAN_DEBUG, ASSERT_NE(__VA_ARGS__),                 ((void)0))
#define DEBUG_ASSERT_LT(...)                 IF(KASPAN_DEBUG, ASSERT_LT(__VA_ARGS__),                 ((void)0))
#define DEBUG_ASSERT_LE(...)                 IF(KASPAN_DEBUG, ASSERT_LE(__VA_ARGS__),                 ((void)0))
#define DEBUG_ASSERT_GT(...)                 IF(KASPAN_DEBUG, ASSERT_GT(__VA_ARGS__),                 ((void)0))
#define DEBUG_ASSERT_GE(...)                 IF(KASPAN_DEBUG, ASSERT_GE(__VA_ARGS__),                 ((void)0))
#define DEBUG_ASSERT_NOT(...)                IF(KASPAN_DEBUG, ASSERT_NOT(__VA_ARGS__),                ((void)0))
#define DEBUG_ASSERT_EQ(...)                 IF(KASPAN_DEBUG, ASSERT_EQ(__VA_ARGS__),                 ((void)0))
#define DEBUG_ASSERT_NULLPTR(...)            IF(KASPAN_DEBUG, ASSERT_NULLPTR(__VA_ARGS__),            ((void)0))
#define DEBUG_ASSERT_POINTER(...)            IF(KASPAN_DEBUG, ASSERT_POINTER(__VA_ARGS__),            ((void)0))
#define DEBUG_ASSERT_SIZE_POINTER(...)       IF(KASPAN_DEBUG, ASSERT_SIZE_POINTER(__VA_ARGS__),       ((void)0))
#define DEBUG_ASSERT_IN_RANGE(...)           IF(KASPAN_DEBUG, ASSERT_IN_RANGE(__VA_ARGS__),           ((void)0))
#define DEBUG_ASSERT_IN_RANGE_INCLUSIVE(...) IF(KASPAN_DEBUG, ASSERT_IN_RANGE_INCLUSIVE(__VA_ARGS__), ((void)0))

// clang-format on

namespace internal {

template<typename T>
concept formattable_concept = requires { std::formatter<std::remove_cvref_t<T>>{}; };

[[noreturn]] constexpr void
fail(
  std::string_view head)
{
  if (std::is_constant_evaluated()) throw std::runtime_error{ head.data() };
  std::fwrite(head.data(), 1, head.size(), stderr);
  std::fputc('\n', stderr);
  debug_break();
  std::abort();
}

template<formattable_concept... Args>
[[noreturn]] constexpr void
fail_fmt(
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (std::is_constant_evaluated()) fail(head); // formatting is not constexpr (yet)
  try {
    auto const details = std::format(fmt, std::forward<Args>(args)...);
    std::print(stderr, "{}\n{}\n", head, details);
  } catch (std::format_error& /* e */) {
    std::fwrite(head.data(), 1, head.size(), stderr);
    std::fputc('\n', stderr);
  }
  debug_break();
  std::abort();
}

template<formattable_concept Arg0,
         formattable_concept Arg1,
         formattable_concept... Args>
[[noreturn]] constexpr void
fail_fmt2_fmt(
  std::string_view            head,
  std::format_string<Arg0,
                     Arg1>    fmt2,
  Arg0&&                      arg0,
  Arg1&&                      arg1,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (std::is_constant_evaluated()) fail(head); // formatting is not constexpr (yet)
  try {
    auto const evaluation = std::format(fmt2, std::forward<Arg0>(arg0), std::forward<Arg1>(arg1));
    auto const details    = std::format(fmt, std::forward<Args>(args)...);
    std::print(stderr, "{}\n{}\n{}\n", head, evaluation, details);
  } catch (std::format_error& /* e */) {
    std::fwrite(head.data(), 1, head.size(), stderr);
    std::fputc('\n', stderr);
  }
  debug_break();
  std::abort();
}

template<formattable_concept Arg0,
         formattable_concept Arg1,
         formattable_concept Arg2,
         formattable_concept... Args>
[[noreturn]] constexpr void
fail_fmt3_fmt(
  std::string_view            head,
  std::format_string<Arg0,
                     Arg1,
                     Arg2>    fmt3,
  Arg0&&                      arg0,
  Arg1&&                      arg1,
  Arg2&&                      arg2,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (std::is_constant_evaluated()) fail(head); // formatting is not constexpr (yet)
  try {
    auto const evaluation = std::format(fmt3, std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));
    auto const details    = std::format(fmt, std::forward<Args>(args)...);
    std::print(stderr, "{}\n{}\n{}\n", head, evaluation, details);
  } catch (std::format_error& /* e */) {
    std::fwrite(head.data(), 1, head.size(), stderr);
    std::fputc('\n', stderr);
  }
  debug_break();
  std::abort();
}

template<typename Lhs,
         typename Rhs>
  requires(!arithmetic_concept<Lhs> || !arithmetic_concept<Rhs>)
constexpr bool
eq(
  Lhs const& lhs,
  Rhs const& rhs) noexcept
{
  return lhs == rhs;
}

template<typename Lhs,
         typename Rhs>
  requires(!arithmetic_concept<Lhs> || !arithmetic_concept<Rhs>)
constexpr bool
lt(
  Lhs const& lhs,
  Rhs const& rhs) noexcept
{
  return lhs < rhs;
}

template<arithmetic_concept Lhs,
         arithmetic_concept Rhs>
constexpr bool
eq(
  Lhs lhs,
  Rhs rhs) noexcept
{
  if constexpr (signed_concept<Lhs> == signed_concept<Rhs>) return lhs == rhs;
  else if constexpr (signed_concept<Lhs>) return lhs >= 0 && std::make_unsigned_t<Lhs>(lhs) == rhs;
  else return rhs >= 0 && std::make_unsigned_t<Rhs>(rhs) == lhs;
}

template<arithmetic_concept Lhs,
         arithmetic_concept Rhs>
constexpr bool
lt(
  Lhs lhs,
  Rhs rhs) noexcept
{
  if constexpr (signed_concept<Lhs> == signed_concept<Rhs>) return lhs < rhs;
  else if constexpr (signed_concept<Lhs>) return lhs < 0 || std::make_unsigned_t<Lhs>(lhs) < rhs;
  else return rhs >= 0 && lhs < std::make_unsigned_t<Rhs>(rhs);
}

constexpr bool
ne(
  auto const& lhs,
  auto const& rhs) noexcept
{
  return !eq(lhs, rhs);
}

constexpr bool
gt(
  auto const& lhs,
  auto const& rhs) noexcept
{
  return lt(rhs, lhs);
}

constexpr bool
le(
  auto const& lhs,
  auto const& rhs) noexcept
{
  return !gt(lhs, rhs);
}

constexpr bool
ge(
  auto const& lhs,
  auto const& rhs) noexcept
{
  return !lt(lhs, rhs);
}

constexpr bool
in_range(
  auto const& val,
  auto const& beg,
  auto const& end) noexcept
{
  return ge(val, beg) & lt(val, end);
}

constexpr bool
in_range_inclusive(
  auto const& val,
  auto const& beg,
  auto const& end) noexcept
{
  return ge(val, beg) & le(val, end);
}

}

constexpr void
assert_true(
  auto const&      cond,
  std::string_view head)
{
  if (!cond) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept... Args>
constexpr void
assert_true(
  auto const&                 cond,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (!cond) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

constexpr void
assert_false(
  auto             cond,
  std::string_view head)
{
  if (cond) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept... Args>
constexpr void
assert_false(
  auto const&                 cond,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (cond) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Lhs,
         typename Rhs>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_eq(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::ne(lhs, rhs)) [[unlikely]]
    internal::fail(head);
}

template<typename Lhs,
         typename Rhs,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_eq(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::ne(lhs, rhs)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs>
constexpr void
assert_eq(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::ne(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} != {}", lhs, rhs);
  }
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs,
         internal::formattable_concept... Args>
constexpr void
assert_eq(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::ne(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt2_fmt(head, "  Evaluation : {} != {}", lhs, rhs, fmt, std::forward<Args>(args)...);
  }
}

template<typename Lhs,
         typename Rhs,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_ne(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::eq(lhs, rhs)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Lhs,
         typename Rhs>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_ne(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::eq(lhs, rhs)) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs,
         internal::formattable_concept... Args>
constexpr void
assert_ne(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::eq(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt2_fmt(head, "  Evaluation : {} == {}", lhs, rhs, fmt, std::forward<Args>(args)...);
  }
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs>
constexpr void
assert_ne(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::eq(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} == {}", lhs, rhs);
  }
}

template<typename Lhs,
         typename Rhs,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_lt(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::ge(lhs, rhs)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Lhs,
         typename Rhs>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_lt(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::ge(lhs, rhs)) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs,
         internal::formattable_concept... Args>
constexpr void
assert_lt(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::ge(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt2_fmt(head, "  Evaluation : {} >= {}", lhs, rhs, fmt, std::forward<Args>(args)...);
  }
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs>
constexpr void
assert_lt(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::ge(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} >= {}", lhs, rhs);
  }
}

template<typename Lhs,
         typename Rhs,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_gt(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::le(lhs, rhs)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Lhs,
         typename Rhs>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_gt(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::le(lhs, rhs)) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs,
         internal::formattable_concept... Args>
constexpr void
assert_gt(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::le(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt2_fmt(head, "  Evaluation : {} <= {}", lhs, rhs, fmt, std::forward<Args>(args)...);
  }
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs>
constexpr void
assert_gt(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::le(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} <= {}", lhs, rhs);
  }
}

template<typename Lhs,
         typename Rhs,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_le(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::gt(lhs, rhs)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Lhs,
         typename Rhs>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_le(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::gt(lhs, rhs)) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs,
         internal::formattable_concept... Args>
constexpr void
assert_le(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::gt(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt2_fmt(head, "  Evaluation : {} > {}", lhs, rhs, fmt, std::forward<Args>(args)...);
  }
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs>
constexpr void
assert_le(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::gt(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} > {}", lhs, rhs);
  }
}

template<typename Lhs,
         typename Rhs,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_ge(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::lt(lhs, rhs)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Lhs,
         typename Rhs>
  requires(!internal::formattable_concept<Lhs> || !internal::formattable_concept<Rhs>)
constexpr void
assert_ge(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::lt(lhs, rhs)) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs,
         internal::formattable_concept... Args>
constexpr void
assert_ge(
  Lhs const&                  lhs,
  Rhs const&                  rhs,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (internal::lt(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt2_fmt(head, "  Evaluation : {} < {}", lhs, rhs, fmt, std::forward<Args>(args)...);
  }
}

template<internal::formattable_concept Lhs,
         internal::formattable_concept Rhs>
constexpr void
assert_ge(
  Lhs const&       lhs,
  Rhs const&       rhs,
  std::string_view head)
{
  if (internal::lt(lhs, rhs)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} < {}", lhs, rhs);
  }
}

template<typename Val,
         typename Beg,
         typename End,
         internal::formattable_concept... Args>
  requires(!internal::formattable_concept<Val> || !internal::formattable_concept<Beg> || !internal::formattable_concept<End>)
constexpr void
assert_in_range(
  Val const&                  val,
  Beg const&                  beg,
  End const&                  end,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]]
    internal::fail_fmt(head, fmt, std::forward<Args>(args)...);
}

template<typename Val,
         typename Beg,
         typename End>
  requires(!internal::formattable_concept<Val> || !internal::formattable_concept<Beg> || !internal::formattable_concept<End>)
constexpr void
assert_in_range(
  Val const&       val,
  Beg const&       beg,
  End const&       end,
  std::string_view head)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]]
    internal::fail(head);
}

template<internal::formattable_concept Val,
         internal::formattable_concept Beg,
         internal::formattable_concept End,
         internal::formattable_concept... Args>
constexpr void
assert_in_range(
  Val const&                  val,
  Beg const&                  beg,
  End const&                  end,
  std::string_view            head,
  std::format_string<Args...> fmt,
  Args&&... args)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt3_fmt(head, "  Evaluation : {} in [{}, {})", val, beg, end, fmt, std::forward<Args>(args)...);
  }
}

template<internal::formattable_concept Val,
         internal::formattable_concept Beg,
         internal::formattable_concept End>
constexpr void
assert_in_range(
  Val const&       val,
  Beg const&       beg,
  End const&       end,
  std::string_view head)
{
  if (!internal::in_range(val, beg, end)) [[unlikely]] {
    if (std::is_constant_evaluated()) internal::fail(head);
    internal::fail_fmt(head, "  Evaluation : {} in [{}, {})", val, beg, end);
  }
}

} // namespace kaspan
