#pragma once

#include <util/Arithmetic.hpp>

#include <compare>
#include <cstdio>
#include <format>

#if !defined(NDEBUG)
#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#define DEBUG_BREAK() ((void)0)
#endif

template<typename U, typename T>
auto
deferred_static_cast(T const& t) -> U
{
  return static_cast<U>(t);
}

template<typename U, typename T>
auto
deferred_static_cast(T&& t) -> U
{
  return static_cast<U>(t);
}

#define ASSERT_CMP(OP, LHS, RHS, ...)                                                                                             \
  {                                                                                                                               \
    auto const& lhs = LHS;                                                                                                        \
    auto const& rhs = RHS;                                                                                                        \
    if (not(lhs OP rhs)) [[unlikely]] {                                                                                           \
      std::fputs("[ASSERTION FAILED]\n", stderr);                                                                                 \
      std::fprintf(stderr, "  Condition  : %s " #OP " %s\n", #LHS, #RHS);                                                         \
      std::fprintf(stderr, "  File       : %s\n", __FILE__);                                                                      \
      std::fprintf(stderr, "  Line       : %d\n", __LINE__);                                                                      \
      if constexpr (UnsignedConcept<decltype(lhs)> and UnsignedConcept<decltype(rhs)>)                                            \
        std::fprintf(stderr, "  Evaluation : %lu " #OP " %lu\n", deferred_static_cast<u64>(LHS), deferred_static_cast<u64>(RHS)); \
      if constexpr (SignedConcept<decltype(lhs)> and SignedConcept<decltype(rhs)>)                                                \
        std::fprintf(stderr, "  Evaluation : %ld " #OP " %ld\n", deferred_static_cast<i64>(LHS), deferred_static_cast<i64>(RHS)); \
      if constexpr (SignedConcept<decltype(lhs)> and UnsignedConcept<decltype(rhs)>)                                              \
        std::fprintf(stderr, "  Evaluation : %ld " #OP " %lu\n", deferred_static_cast<i64>(LHS), deferred_static_cast<u64>(RHS)); \
      if constexpr (UnsignedConcept<decltype(lhs)> and SignedConcept<decltype(rhs)>)                                              \
        std::fprintf(stderr, "  Evaluation : %lu " #OP " %ld\n", deferred_static_cast<u64>(LHS), deferred_static_cast<i64>(RHS)); \
      if constexpr (FloatConcept<decltype(lhs)> and FloatConcept<decltype(rhs)>)                                                  \
        std::fprintf(stderr, "  Evaluation : %f " #OP " %f\n", deferred_static_cast<f64>(LHS), deferred_static_cast<f64>(RHS));   \
      __VA_OPT__(std::fprintf(stderr, "  Details    : " __VA_ARGS__); std::fputc('\n', stderr);)                                  \
      DEBUG_BREAK();                                                                                                              \
      std::abort();                                                                                                               \
    }                                                                                                                             \
  }                                                                                                                               \
  ((void)0)

#define ASSERT(COND, ...)                                                                      \
  if (not(COND)) [[unlikely]] {                                                                \
    std::fputs("[ASSERTION FAILED]\n", stderr);                                                \
    std::fprintf(stderr, "  Condition  : %s\n", #COND);                                        \
    std::fprintf(stderr, "  File       : %s\n", __FILE__);                                     \
    std::fprintf(stderr, "  Line       : %d\n", __LINE__);                                     \
    __VA_OPT__(std::fprintf(stderr, "  Details    : " __VA_ARGS__); std::fputc('\n', stderr);) \
    DEBUG_BREAK();                                                                             \
    std::abort();                                                                              \
  }                                                                                            \
  ((void)0)

#define ASSERT_LT(LHS, RHS, ...) ASSERT_CMP(<, LHS, RHS, __VA_ARGS__)
#define ASSERT_GT(LHS, RHS, ...) ASSERT_CMP(>, LHS, RHS, __VA_ARGS__)
#define ASSERT_LE(LHS, RHS, ...) ASSERT_CMP(<=, LHS, RHS, __VA_ARGS__)
#define ASSERT_GE(LHS, RHS, ...) ASSERT_CMP(>=, LHS, RHS, __VA_ARGS__)
#define ASSERT_EQ(LHS, RHS, ...) ASSERT_CMP(==, LHS, RHS, __VA_ARGS__)
#define ASSERT_NE(LHS, RHS, ...) ASSERT_CMP(!=, LHS, RHS, __VA_ARGS__)
