#pragma once

#include <pp/PP.hpp>
#include <test/Assert.hpp>
#include <util/Arithmetic.hpp>

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

enum class ErrorCode : u8
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
  ASSERTION_ERROR      = 9,
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
  constexpr Result(T&& value) // NOLINT(*-explicit-constructor, *-explicit-conversions)
    : data(std::in_place_index<0>, std::forward<T>(value))
  {
  }

  template<typename T>
    requires(error_convertible_v<T>)
  constexpr Result(T&& error) // NOLINT(*-explicit-constructor, *-explicit-conversions)
    : data(std::in_place_index<1>, std::forward<T>(error))
  {
  }

  static constexpr auto success() -> Result
    requires(value_is_void_v)
  {
    return Result();
  }

  static constexpr auto success(V value) -> Result
  {
    return Result(std::move(value));
  }

  static constexpr auto failure() -> Result
  {
    return Result(ErrorCode::ERROR);
  }

  static constexpr auto failure(ErrorCode error) -> Result
  {
    return Result(error);
  }

  [[nodiscard]] constexpr auto has_value() const -> bool
  {
    return std::holds_alternative<V>(data);
  }

  constexpr explicit operator bool() const
  {
    return has_value();
  }

  constexpr auto error() -> ErrorCode
  {
    return std::get<ErrorCode>(data);
  }

  [[nodiscard]] constexpr auto error() const -> ErrorCode
  {
    return std::get<ErrorCode>(data);
  }

  constexpr auto value() & -> V&
  {
    return std::get<V>(data);
  }

  constexpr auto value() const& -> V const&
  {
    return std::get<V>(data);
  }

  constexpr auto value() && -> V&&
  {
    return std::move(std::get<V>(data));
  }

  constexpr auto value() const&& -> V const&&
  {
    return std::move(std::get<V>(data));
  }

  template<class U>
    requires(value_convertible_v<U>)
  constexpr auto value_or(U&& alt) const& -> V
  {
    return has_value() ? value() : V{ std::forward<U>(alt) };
  }
  template<class U>
    requires(value_convertible_v<U>)
  constexpr auto value_or(U&& alt) && -> V
  {
    return has_value() ? std::move(*this).value() : V{ std::forward<U>(alt) };
  }

  [[nodiscard]] constexpr auto error_or_ok() const -> ErrorCode
    requires(value_is_void_v)
  {
    return has_value() ? ErrorCode::OK : error();
  }

  template<class F>
  [[nodiscard]] constexpr auto map(F&& mapper) const
  {
    using R = Result<std::invoke_result_t<F, V const&>>;
    if (not has_value())
      return R::failure(error());
    return R::success(std::invoke(std::forward<F>(mapper), value()));
  }

  template<class F>
  [[nodiscard]] constexpr auto and_then(F&& consumer) const
  {
    using R = std::invoke_result_t<F, V const&>;
    if (not has_value())
      return R::failure(error());
    return std::invoke(std::forward<F>(consumer), value());
  }

  auto operator==(Result const&) const -> bool = default;
  auto operator!=(Result const&) const -> bool = default;
};

#define RESULT_TRY_1(EXPR, TMP)                       \
  auto&& TMP = (EXPR);                                \
  if (!(TMP)) { /* NOLINT(*-simplify-boolean-expr) */ \
    return (TMP).error();                             \
  }                                                   \
  ((void)0)
#define RESULT_TRY_2(VAR, EXPR, TMP) \
  RESULT_TRY_1(EXPR, TMP);           \
  VAR = std::move((TMP).value())
#define RESULT_TRY(...) CAT(RESULT_TRY_, ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__, CAT(tmp, __COUNTER__))

#define RESULT_ASSERT_1(COND) \
  RESULT_ASSERT_2(COND, ASSERTION_ERROR)
#define RESULT_ASSERT_2(COND, ERROR_CODE)                \
  if (not(COND)) { /* NOLINT(*-simplify-boolean-expr) */ \
    return ErrorCode::ERROR_CODE;                        \
  }                                                      \
  ((void)0)
#define RESULT_ASSERT_3(COND, ERROR_CODE, ERROR_STRING)  \
  if (not(COND)) { /* NOLINT(*-simplify-boolean-expr) */ \
    perror(ERROR_STRING);                                \
    return (ErrorCode::ERROR_CODE);                      \
  }                                                      \
  ((void)0)
#define RESULT_ASSERT(...) CAT(RESULT_ASSERT_, ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)

#define ASSERT_TRY_1(EXPR, TMP) \
  auto&& TMP = (EXPR);          \
  ASSERT(TMP)
#define ASSERT_TRY_2(VAR, EXPR, TMP) \
  ASSERT_TRY_1(EXPR, TMP);           \
  VAR = std::move(TMP.value())
#define ASSERT_TRY(...) CAT(ASSERT_TRY_, ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__, CAT(tmp, __COUNTER__))

using VoidResult = Result<std::monostate>;
