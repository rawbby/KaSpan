#pragma once

#include <cassert>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace ka_span {

template<typename V, typename E>
class [[nodiscard]] result
{
  std::variant<V, E> data;

  template<class T>
  static constexpr bool value_convertible_v = std::convertible_to<T&&, V> and !std::convertible_to<T&&, E>;

  template<class T>
  static constexpr bool error_convertible_v = std::convertible_to<T&&, E> and !std::convertible_to<T&&, V>;

public:
  using value_type = V;
  using error_type = E;

  /*----------- construction -----------*/

  constexpr result()
    requires(std::is_same_v<V, std::monostate>)
    : data(std::in_place_index<0>, std::monostate())
  {
  }

  template<typename T>
    requires(value_convertible_v<T>)
  constexpr result(T&& value) // NOLINT(*-explicit-constructor)
    : data(std::in_place_index<0>, std::forward<T>(value))
  {
  }

  template<typename T>
    requires(error_convertible_v<T>)
  constexpr result(T&& error) // NOLINT(*-explicit-constructor)
    : data(std::in_place_index<1>, std::forward<T>(error))
  {
  }

  // clang-format off
  constexpr result(const V& value) : data(value)            {} // NOLINT(*-explicit-constructor)
  constexpr result(V&& value)      : data(std::move(value)) {} // NOLINT(*-explicit-constructor)
  constexpr result(const E& error) : data(error)            {} // NOLINT(*-explicit-constructor)
  constexpr result(E&& error)      : data(std::move(error)) {} // NOLINT(*-explicit-constructor)
  // clang-format on

  static constexpr result success()
    requires(std::is_same_v<V, std::monostate>)
  {
    return result();
  }

  static constexpr result success(V value)
  {
    return result(std::move(value));
  }

  static constexpr result failure(E error)
  {
    return result(std::move(error));
  }

  /*----------- observers -------------*/

  [[nodiscard]] constexpr bool has_value() const
  {
    return std::holds_alternative<V>(data);
  }

  constexpr explicit operator bool() const
  {
    return has_value();
  }

  constexpr V& value()
  {
    assert(has_value());
    return std::get<V>(data);
  }

  [[nodiscard]] constexpr const V& value() const
  {
    assert(has_value());
    return std::get<V>(data);
  }

  constexpr E& error()
  {
    assert(!has_value());
    return std::get<E>(data);
  }

  [[nodiscard]] constexpr const E& error() const
  {
    assert(!has_value());
    return std::get<E>(data);
  }

  /*----------- value_or --------------*/

  template<class U>
  constexpr V value_or(U&& alt) const
    requires(std::convertible_to<U, V>)
  {
    return has_value() ? value() : static_cast<V>(std::forward<U>(alt));
  }

  template<class U>
  constexpr E error_or(U&& alt) const
    requires(std::convertible_to<U, E>)
  {
    return has_value() ? static_cast<E>(std::forward<U>(alt)) : error();
  }

  /*----------- map -------------------*/

  template<class F>
  constexpr auto map(F&& f) const
  {

    using R = result<std::invoke_result_t<F, const V&>, E>;
    if (has_value())
      return R::success(std::invoke(std::forward<F>(f), value()));
    return R::failure(error());
  }

  /*----------- and_then --------------*/

  template<class F>
  [[nodiscard]] constexpr auto and_then(F&& f) const
  {

    using R = std::invoke_result_t<F, const V&>;
    if (has_value())
      return std::invoke(std::forward<F>(f), value());
    return R::failure(error());
  }

  /*----------- comparison ------------*/
  constexpr bool operator==(const result&) const = default;
};

#define RESULT_TRY_DISPATCH0(_1, _2, NAME, ...) NAME
#define RESULT_TRY_CAT1(A, B) A##B
#define RESULT_TRY_CAT0(A, B) RESULT_TRY_CAT1(A, B)

#define RESULT_TRY1(EXPR, TMP)       \
  do {                               \
    auto&& TMP = (EXPR);             \
    if (!TMP)                        \
      return std::move(TMP.error()); \
  } while (false)

#define RESULT_TRY2(VAR, EXPR, TMP) \
  auto&& TMP = (EXPR);              \
  if (!TMP)                         \
    return std::move(TMP.error());  \
  VAR = std::move(TMP.value())

#define RESULT_TRY(...) \
  RESULT_TRY_DISPATCH0(__VA_ARGS__, RESULT_TRY2, RESULT_TRY1)(__VA_ARGS__, RESULT_TRY_CAT0(tmp, __COUNTER__))

enum error_code : int
{
  OK    = 0,
  ERROR = -1,

  DESERIALIZE_ERROR    = 1,
  SERIALIZE_ERROR      = 2,
  IO_ERROR             = 3,
  MEMORY_MAPPING_ERROR = 4,
  ALLOCATION_ERROR     = 5,
  ASSUMPTION_ERROR     = 6,
  FILESYSTEM_ERROR     = 7,
};

template<typename T>
using ka_result      = result<T, error_code>;
using ka_void_result = result<std::monostate, error_code>;

#define VALUE_TRY0(VAR, EXPR, ERROR_CODE, TMP)  \
  auto&& TMP = (EXPR);                          \
  if (not(TMP))                                 \
    return (::ka_span::error_code::ERROR_CODE); \
  VAR = std::move(TMP)

#define VALUE_TRY(VAR, EXPR, ERROR_CODE) VALUE_TRY0(VAR, EXPR, ERROR_CODE, RESULT_TRY_CAT0(tmp, __COUNTER__))

#define ERROR_CODE_TRY(EXPR, ERROR_CODE) \
  if (EXPR)                              \
  return (::ka_span::error_code::ERROR_CODE)

#define ASSERT_TRY(COND, ERROR_CODE) \
  if (not(COND))                     \
  return (::ka_span::error_code::ERROR_CODE)

} // namespace ka_span
