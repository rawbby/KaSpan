#pragma once

#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

enum class ErrorCode : std::uint8_t
{
  OK    = 0,
  ERROR = 1,

  DESERIALIZE_ERROR    = 2,
  SERIALIZE_ERROR      = 3,
  IO_ERROR             = 4,
  MEMORY_MAPPING_ERROR = 5,
  ALLOCATION_ERROR     = 6,
  ASSUMPTION_ERROR     = 7,
  FILESYSTEM_ERROR     = 8,
};

template<typename V>
class Result final
{
  std::variant<V, ErrorCode> data;

  static constexpr bool value_is_void_v = std::is_same_v<V, std::monostate>;

  template<class T>
  static constexpr bool value_convertible_v = std::convertible_to<T&&, V> and not std::convertible_to<T&&, ErrorCode>;

  template<class T>
  static constexpr bool error_convertible_v = std::convertible_to<T&&, ErrorCode> and not std::convertible_to<T&&, V>;

public:
  using value_type = V;

  constexpr Result()
    requires(value_is_void_v)
    : data(std::in_place_index<0>, std::monostate{})
  {
  }

  template<typename T>
    requires(value_convertible_v<T>)
  constexpr Result(T&& value)
    : data(std::in_place_index<0>, std::forward<T>(value))
  {
  }

  template<typename T>
    requires(error_convertible_v<T>)
  constexpr Result(T&& error)
    : data(std::in_place_index<1>, std::forward<T>(error))
  {
  }

  static constexpr Result success()
    requires(value_is_void_v)
  {
    return Result();
  }

  static constexpr Result success(V value)
  {
    return Result(std::move(value));
  }

  static constexpr Result failure()
  {
    return Result(ErrorCode::ERROR);
  }

  static constexpr Result failure(ErrorCode error)
  {
    return Result(error);
  }

  [[nodiscard]] constexpr bool has_value() const
  {
    return std::holds_alternative<V>(data);
  }

  constexpr explicit operator bool() const
  {
    return has_value();
  }

  constexpr ErrorCode error()
  {
    assert(not has_value());
    return std::get<ErrorCode>(data);
  }

  [[nodiscard]] constexpr ErrorCode error() const
  {
    assert(not has_value());
    return std::get<ErrorCode>(data);
  }

  constexpr V& value() &
  {
    assert(has_value());
    return std::get<V>(data);
  }

  constexpr const V& value() const&
  {
    assert(has_value());
    return std::get<V>(data);
  }

  constexpr V&& value() &&
  {
    assert(has_value());
    return std::move(std::get<V>(data));
  }

  constexpr const V&& value() const&&
  {
    assert(has_value());
    return std::move(std::get<V>(data));
  }

  template<class U>
    requires(value_convertible_v<U>)
  constexpr V value_or(U&& alt) const&
  {
    return has_value() ? value() : V{ std::forward<U>(alt) };
  }
  template<class U>
    requires(value_convertible_v<U>)
  constexpr V value_or(U&& alt) &&
  {
    return has_value() ? std::move(*this).value() : V{ std::forward<U>(alt) };
  }

  constexpr ErrorCode error_or_ok() const
    requires(value_is_void_v)
  {
    return has_value() ? ErrorCode::OK : error();
  }

  template<class F>
  constexpr auto map(F&& f) const
  {
    using R = Result<std::invoke_result_t<F, const V&>>;
    if (not has_value())
      return R::failure(error());
    return R::success(std::invoke(std::forward<F>(f), value()));
  }

  template<class F>
  [[nodiscard]] constexpr auto and_then(F&& f) const
  {
    using R = std::invoke_result_t<F, const V&>;
    if (not has_value())
      return R::failure(error());
    return std::invoke(std::forward<F>(f), value());
  }

  constexpr bool operator==(const Result&) const = default;
};

#define RESULT_TRY_CAT2(A, B) A##B
#define RESULT_TRY_CAT_(A, B) RESULT_TRY_CAT2(A, B)

#define RESULT_TRY_DISPATCH_(_1, _2, NAME, ...) NAME
#define RESULT_TRY2(EXPR, TMP) \
  do {                         \
    auto&& TMP = (EXPR);       \
    if (!TMP) {                \
      return TMP.error();      \
    }                          \
  } while (false)
#define RESULT_TRY3(VAR, EXPR, TMP) \
  auto&& TMP = (EXPR);              \
  if (!TMP) {                       \
    return TMP.error();             \
  }                                 \
  VAR = std::move(TMP).value();
#define RESULT_TRY(...) \
  RESULT_TRY_DISPATCH_(__VA_ARGS__, RESULT_TRY3, RESULT_TRY2, )(__VA_ARGS__, RESULT_TRY_CAT_(tmp, __COUNTER__))

#define ASSERT_TRY_DISPATCH_(_1, _2, _3, NAME, ...) NAME
#define ASSERT_TRY2(COND, ERROR_CODE) \
  do {                                \
    if (not(COND)) {                  \
      return (ErrorCode::ERROR_CODE); \
    }                                 \
  } while (false)
#define ASSERT_TRY3(COND, ERROR_CODE, ERROR_STRING) \
  do {                                              \
    if (not(COND)) {                                \
      perror(ERROR_STRING);                         \
      return (ErrorCode::ERROR_CODE);               \
    }                                               \
  } while (false)
#define ASSERT_TRY(...) \
  ASSERT_TRY_DISPATCH_(__VA_ARGS__, ASSERT_TRY3, ASSERT_TRY2, _)(__VA_ARGS__)

using VoidResult = Result<std::monostate>;
