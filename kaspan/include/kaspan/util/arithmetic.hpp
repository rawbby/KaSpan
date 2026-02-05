#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

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

// clang-format off
template<typename T> concept byte_c = std::same_as<std::remove_cvref_t<T>, byte>;

template<typename T> concept unsigned_c = !std::is_enum_v<T> && std::unsigned_integral<std::remove_cvref_t<T>>;
template<typename T> concept signed_c   = !std::is_enum_v<T> && std::signed_integral<std::remove_cvref_t<T>>;

template<typename T> concept integral_c  = unsigned_c<T> || signed_c<T>;
template<typename T> concept u64_c       = unsigned_c<T> && sizeof(T) == 8;
template<typename T> concept u32_c       = unsigned_c<T> && sizeof(T) == 4;
template<typename T> concept u16_c       = unsigned_c<T> && sizeof(T) == 2;
template<typename T> concept u8_c        = unsigned_c<T> && sizeof(T) == 1;
template<typename T> concept i64_c       = signed_c<T>   && sizeof(T) == 8;
template<typename T> concept i32_c       = signed_c<T>   && sizeof(T) == 4;
template<typename T> concept i16_c       = signed_c<T>   && sizeof(T) == 2;
template<typename T> concept i8_c        = signed_c<T>   && sizeof(T) == 1;

template<typename T> concept float_c = ! std::is_enum_v<T> && (
  std::same_as<std::remove_cvref_t<T>, float>  ||
  std::same_as<std::remove_cvref_t<T>, double> ||
  std::same_as<std::remove_cvref_t<T>, long double>);

template<typename T> concept f32_c = float_c<T> && sizeof(T) == 4;
template<typename T> concept f64_c = float_c<T> && sizeof(T) == 8;

template<typename T> concept arithmetic_c  = integral_c<T> || float_c<T>;

// clang-format on

} // namespace kaspan
