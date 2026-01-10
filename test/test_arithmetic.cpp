#include <concepts>
#include <cstdint>
#include <kaspan/util/arithmetic.hpp>
#include <type_traits>
#include <variant>

using namespace kaspan;

int
main()
{
  static_assert(i8_concept<char>);
  static_assert(i8_concept<signed char>);
  static_assert(u8_concept<unsigned char>);

  static_assert(i16_concept<short>);
  static_assert(i16_concept<signed short>);
  static_assert(u16_concept<unsigned short>);

  static_assert(i32_concept<int>);
  static_assert(i32_concept<signed int>);
  static_assert(u32_concept<unsigned int>);

  static_assert(i64_concept<long long>);
  static_assert(i64_concept<signed long long>);
  static_assert(u64_concept<unsigned long long>);

  static_assert(f32_concept<float>);
  static_assert(f64_concept<double>);

  static_assert(byte_concept<byte>);
  static_assert(not i8_concept<byte>);
  static_assert(not u8_concept<byte>);
  static_assert(i8_concept<std::int8_t>);
  static_assert(u8_concept<std::uint8_t>);
  static_assert(i16_concept<std::int16_t>);
  static_assert(u16_concept<std::uint16_t>);
  static_assert(i32_concept<std::int32_t>);
  static_assert(u32_concept<std::uint32_t>);
  static_assert(i64_concept<std::int64_t>);
  static_assert(u64_concept<std::uint64_t>);

  static_assert(not has_uint128_t or u128_concept<u128>);
  static_assert(not has_int128_t or i128_concept<i128>);

  static_assert(std::same_as<byte, byte>);
  static_assert(std::same_as<u8, std::uint8_t>);
  static_assert(std::same_as<u16, std::uint16_t>);
  static_assert(std::same_as<u32, std::uint32_t>);
  static_assert(std::same_as<u64, std::uint64_t>);
  static_assert(std::same_as<i8, std::int8_t>);
  static_assert(std::same_as<i16, std::int16_t>);
  static_assert(std::same_as<i32, std::int32_t>);
  static_assert(std::same_as<i64, std::int64_t>);

  static_assert(byte_concept<byte>);
  static_assert(u8_concept<u8>);
  static_assert(u16_concept<u16>);
  static_assert(u32_concept<u32>);
  static_assert(u64_concept<u64>);
  static_assert(i8_concept<i8>);
  static_assert(i16_concept<i16>);
  static_assert(i32_concept<i32>);
  static_assert(i64_concept<i64>);

  static_assert(float_concept<f32>);
  static_assert(float_concept<f64>);

  static_assert(not unsigned_concept<byte>);
  static_assert(unsigned_concept<u8>);
  static_assert(unsigned_concept<u16>);
  static_assert(unsigned_concept<u32>);
  static_assert(unsigned_concept<u64>);

  static_assert(signed_concept<i8>);
  static_assert(signed_concept<i16>);
  static_assert(signed_concept<i32>);
  static_assert(signed_concept<i64>);

  static_assert(not integral_concept<byte>);
  static_assert(integral_concept<u8>);
  static_assert(integral_concept<u16>);
  static_assert(integral_concept<u32>);
  static_assert(integral_concept<u64>);
  static_assert(integral_concept<i8>);
  static_assert(integral_concept<i16>);
  static_assert(integral_concept<i32>);
  static_assert(integral_concept<i64>);

  static_assert(not arithmetic_concept<byte>);
  static_assert(arithmetic_concept<u8>);
  static_assert(arithmetic_concept<u16>);
  static_assert(arithmetic_concept<u32>);
  static_assert(arithmetic_concept<u64>);
  static_assert(arithmetic_concept<i8>);
  static_assert(arithmetic_concept<i16>);
  static_assert(arithmetic_concept<i32>);
  static_assert(arithmetic_concept<i64>);
  static_assert(arithmetic_concept<f32>);
  static_assert(arithmetic_concept<f64>);

  static_assert(has_uint128_t == u128_concept<u128>);
  static_assert(has_int128_t == i128_concept<i128>);

  static_assert(has_uint128_t or std::is_same_v<u128, std::monostate>);
  static_assert(has_int128_t or std::is_same_v<i128, std::monostate>);

  static_assert(u32_concept<u32>);
  static_assert(u32_concept<u32&>);
  static_assert(u32_concept<u32&&>);
  static_assert(u32_concept<u32 const>);
  static_assert(u32_concept<u32 volatile>);
  static_assert(u32_concept<u32 const&>);
  static_assert(u32_concept<u32 const&&>);
  static_assert(u32_concept<u32 volatile&>);
  static_assert(u32_concept<u32 volatile&&>);
  static_assert(u32_concept<u32 const volatile&>);
  static_assert(u32_concept<u32 const volatile&&>);

  static_assert(i32_concept<i32>);
  static_assert(i32_concept<i32&>);
  static_assert(i32_concept<i32&&>);
  static_assert(i32_concept<i32 const>);
  static_assert(i32_concept<i32 volatile>);
  static_assert(i32_concept<i32 const&>);
  static_assert(i32_concept<i32 const&&>);
  static_assert(i32_concept<i32 volatile&>);
  static_assert(i32_concept<i32 volatile&&>);
  static_assert(i32_concept<i32 const volatile&>);
  static_assert(i32_concept<i32 const volatile&&>);

  static_assert(f32_concept<f32>);
  static_assert(f32_concept<f32&>);
  static_assert(f32_concept<f32&&>);
  static_assert(f32_concept<f32 const>);
  static_assert(f32_concept<f32 volatile>);
  static_assert(f32_concept<f32 const&>);
  static_assert(f32_concept<f32 const&&>);
  static_assert(f32_concept<f32 volatile&>);
  static_assert(f32_concept<f32 volatile&&>);
  static_assert(f32_concept<f32 const volatile&>);
  static_assert(f32_concept<f32 const volatile&&>);

  static_assert(integral_concept<i64>);
  static_assert(integral_concept<i64&>);
  static_assert(integral_concept<i64&&>);
  static_assert(integral_concept<i64 const>);
  static_assert(integral_concept<i64 volatile>);
  static_assert(integral_concept<i64 const&>);
  static_assert(integral_concept<i64 const&&>);
  static_assert(integral_concept<i64 volatile&>);
  static_assert(integral_concept<i64 volatile&&>);
  static_assert(integral_concept<i64 const volatile&>);
  static_assert(integral_concept<i64 const volatile&&>);

  static_assert(float_concept<f64>);
  static_assert(float_concept<f64&>);
  static_assert(float_concept<f64&&>);
  static_assert(float_concept<f64 const>);
  static_assert(float_concept<f64 volatile>);
  static_assert(float_concept<f64 const&>);
  static_assert(float_concept<f64 const&&>);
  static_assert(float_concept<f64 volatile&>);
  static_assert(float_concept<f64 volatile&&>);
  static_assert(float_concept<f64 const volatile&>);
  static_assert(float_concept<f64 const volatile&&>);

  static_assert(arithmetic_concept<u64>);
  static_assert(arithmetic_concept<u64&>);
  static_assert(arithmetic_concept<u64&&>);
  static_assert(arithmetic_concept<u64 const>);
  static_assert(arithmetic_concept<u64 volatile>);
  static_assert(arithmetic_concept<u64 const&>);
  static_assert(arithmetic_concept<u64 const&&>);
  static_assert(arithmetic_concept<u64 volatile&>);
  static_assert(arithmetic_concept<u64 volatile&&>);
  static_assert(arithmetic_concept<u64 const volatile&>);
  static_assert(arithmetic_concept<u64 const volatile&&>);

  static_assert(arithmetic_concept<f64>);
  static_assert(arithmetic_concept<f64&>);
  static_assert(arithmetic_concept<f64&&>);
  static_assert(arithmetic_concept<f64 const>);
  static_assert(arithmetic_concept<f64 volatile>);
  static_assert(arithmetic_concept<f64 const&>);
  static_assert(arithmetic_concept<f64 const&&>);
  static_assert(arithmetic_concept<f64 volatile&>);
  static_assert(arithmetic_concept<f64 volatile&&>);
  static_assert(arithmetic_concept<f64 const volatile&>);
  static_assert(arithmetic_concept<f64 const volatile&&>);

  static_assert(not f32_concept<f64>);
  static_assert(not f64_concept<f32>);
  static_assert(not i32_concept<i64>);
  static_assert(not i64_concept<i32>);
  static_assert(not u32_concept<u64>);
  static_assert(not u64_concept<u32>);

  static_assert(not integral_concept<f64>);
  static_assert(not integral_concept<f32>);
  static_assert(not float_concept<i64>);
  static_assert(not float_concept<i32>);
  static_assert(not float_concept<u64>);
  static_assert(not float_concept<u32>);
}
