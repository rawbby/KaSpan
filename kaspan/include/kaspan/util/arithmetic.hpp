// ReSharper disable CppUnusedIncludeDirective
// ReSharper disable CppRedundantBooleanExpressionArgument

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <version>

namespace kaspan {

using byte = std::byte;
using u64  = std::uint64_t;
using u32  = std::uint32_t;
using u16  = std::uint16_t;
using u8   = std::uint8_t;
using i64  = std::int64_t;
using i32  = std::int32_t;
using i16  = std::int16_t;
using i8   = std::int8_t;

using f32 = float;
using f64 = double;

// 1) Preferred: standard facility
#if defined(__cpp_lib_int128)
using u128                          = std::uint128_t;
using i128                          = std::int128_t;
inline constexpr bool has_uint128_t = true;
inline constexpr bool has_int128_t  = true;

// 2) Fallback: compiler extension (GCC/Clang)
#elif defined(__SIZEOF_INT128__) and not defined(_MSC_VER)
using u128                          = unsigned __int128;
using i128                          = __int128;
inline constexpr bool has_uint128_t = true;
inline constexpr bool has_int128_t  = true;

// 3) No 128-bit support
#else
using u128                          = std::monostate;
using i128                          = std::monostate;
inline constexpr bool has_uint128_t = false;
inline constexpr bool has_int128_t  = false;

#endif

// clang-format off
template<typename T> concept byte_concept = std::same_as<std::remove_cvref_t<T>, byte>;

template<typename T> concept unsigned_concept = not std::is_enum_v<T> and (
  (std::unsigned_integral<std::remove_cvref_t<T>>) or
  (has_uint128_t and std::same_as<std::remove_cvref_t<T>, u128>));
template<typename T> concept signed_concept = not std::is_enum_v<T> and (
  (std::signed_integral<std::remove_cvref_t<T>>) or
  (has_int128_t and std::same_as<std::remove_cvref_t<T>, i128>));

template<typename T> concept integral_concept  = unsigned_concept<T> or signed_concept<T>;
template<typename T> concept u128_concept = unsigned_concept<T> and (sizeof(T) == 16);
template<typename T> concept u64_concept  = unsigned_concept<T> and (sizeof(T) ==  8);
template<typename T> concept u32_concept  = unsigned_concept<T> and (sizeof(T) ==  4);
template<typename T> concept u16_concept  = unsigned_concept<T> and (sizeof(T) ==  2);
template<typename T> concept u8_concept   = unsigned_concept<T> and (sizeof(T) ==  1);
template<typename T> concept i128_concept = signed_concept<T>   and (sizeof(T) == 16);
template<typename T> concept i64_concept  = signed_concept<T>   and (sizeof(T) ==  8);
template<typename T> concept i32_concept  = signed_concept<T>   and (sizeof(T) ==  4);
template<typename T> concept i16_concept  = signed_concept<T>   and (sizeof(T) ==  2);
template<typename T> concept i8_concept   = signed_concept<T>   and (sizeof(T) ==  1);

template<typename T> concept float_concept = not std::is_enum_v<T> and (
  std::same_as<std::remove_cvref_t<T>, float>  or
  std::same_as<std::remove_cvref_t<T>, double> or
  std::same_as<std::remove_cvref_t<T>, long double>);

template<typename T> concept f32_concept = float_concept<T> and (sizeof(T) == 4);
template<typename T> concept f64_concept = float_concept<T> and (sizeof(T) == 8);

template<typename T> concept arithmetic_concept  = integral_concept<T> or float_concept<T>;
// clang-format on

} // namespace kaspan
