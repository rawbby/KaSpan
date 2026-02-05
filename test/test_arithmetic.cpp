#include <kaspan/util/arithmetic.hpp>

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <variant>

using namespace kaspan;

int
main()
{
  static_assert(i8_c<char>);
  static_assert(i8_c<signed char>);
  static_assert(u8_c<unsigned char>);

  static_assert(i16_c<short>);
  static_assert(i16_c<signed short>);
  static_assert(u16_c<unsigned short>);

  static_assert(i32_c<int>);
  static_assert(i32_c<signed int>);
  static_assert(u32_c<unsigned int>);

  static_assert(i64_c<long long>);
  static_assert(i64_c<signed long long>);
  static_assert(u64_c<unsigned long long>);

  static_assert(f32_c<float>);
  static_assert(f64_c<double>);

  static_assert(byte_c<byte>);
  static_assert(!i8_c<byte>);
  static_assert(!u8_c<byte>);
  static_assert(i8_c<std::int8_t>);
  static_assert(u8_c<std::uint8_t>);
  static_assert(i16_c<std::int16_t>);
  static_assert(u16_c<std::uint16_t>);
  static_assert(i32_c<std::int32_t>);
  static_assert(u32_c<std::uint32_t>);
  static_assert(i64_c<std::int64_t>);
  static_assert(u64_c<std::uint64_t>);

  static_assert(std::same_as<byte, byte>);
  static_assert(std::same_as<u8, std::uint8_t>);
  static_assert(std::same_as<u16, std::uint16_t>);
  static_assert(std::same_as<u32, std::uint32_t>);
  static_assert(std::same_as<u64, std::uint64_t>);
  static_assert(std::same_as<i8, std::int8_t>);
  static_assert(std::same_as<i16, std::int16_t>);
  static_assert(std::same_as<i32, std::int32_t>);
  static_assert(std::same_as<i64, std::int64_t>);

  static_assert(byte_c<byte>);
  static_assert(u8_c<u8>);
  static_assert(u16_c<u16>);
  static_assert(u32_c<u32>);
  static_assert(u64_c<u64>);
  static_assert(i8_c<i8>);
  static_assert(i16_c<i16>);
  static_assert(i32_c<i32>);
  static_assert(i64_c<i64>);

  static_assert(float_c<f32>);
  static_assert(float_c<f64>);

  static_assert(!unsigned_c<byte>);
  static_assert(unsigned_c<u8>);
  static_assert(unsigned_c<u16>);
  static_assert(unsigned_c<u32>);
  static_assert(unsigned_c<u64>);

  static_assert(signed_c<i8>);
  static_assert(signed_c<i16>);
  static_assert(signed_c<i32>);
  static_assert(signed_c<i64>);

  static_assert(!integral_c<byte>);
  static_assert(integral_c<u8>);
  static_assert(integral_c<u16>);
  static_assert(integral_c<u32>);
  static_assert(integral_c<u64>);
  static_assert(integral_c<i8>);
  static_assert(integral_c<i16>);
  static_assert(integral_c<i32>);
  static_assert(integral_c<i64>);

  static_assert(!arithmetic_c<byte>);
  static_assert(arithmetic_c<u8>);
  static_assert(arithmetic_c<u16>);
  static_assert(arithmetic_c<u32>);
  static_assert(arithmetic_c<u64>);
  static_assert(arithmetic_c<i8>);
  static_assert(arithmetic_c<i16>);
  static_assert(arithmetic_c<i32>);
  static_assert(arithmetic_c<i64>);
  static_assert(arithmetic_c<f32>);
  static_assert(arithmetic_c<f64>);

  static_assert(u32_c<u32>);
  static_assert(u32_c<u32&>);
  static_assert(u32_c<u32&&>);
  static_assert(u32_c<u32 const>);
  static_assert(u32_c<u32 volatile>);
  static_assert(u32_c<u32 const&>);
  static_assert(u32_c<u32 const&&>);
  static_assert(u32_c<u32 volatile&>);
  static_assert(u32_c<u32 volatile&&>);
  static_assert(u32_c<u32 const volatile&>);
  static_assert(u32_c<u32 const volatile&&>);

  static_assert(i32_c<i32>);
  static_assert(i32_c<i32&>);
  static_assert(i32_c<i32&&>);
  static_assert(i32_c<i32 const>);
  static_assert(i32_c<i32 volatile>);
  static_assert(i32_c<i32 const&>);
  static_assert(i32_c<i32 const&&>);
  static_assert(i32_c<i32 volatile&>);
  static_assert(i32_c<i32 volatile&&>);
  static_assert(i32_c<i32 const volatile&>);
  static_assert(i32_c<i32 const volatile&&>);

  static_assert(f32_c<f32>);
  static_assert(f32_c<f32&>);
  static_assert(f32_c<f32&&>);
  static_assert(f32_c<f32 const>);
  static_assert(f32_c<f32 volatile>);
  static_assert(f32_c<f32 const&>);
  static_assert(f32_c<f32 const&&>);
  static_assert(f32_c<f32 volatile&>);
  static_assert(f32_c<f32 volatile&&>);
  static_assert(f32_c<f32 const volatile&>);
  static_assert(f32_c<f32 const volatile&&>);

  static_assert(integral_c<i64>);
  static_assert(integral_c<i64&>);
  static_assert(integral_c<i64&&>);
  static_assert(integral_c<i64 const>);
  static_assert(integral_c<i64 volatile>);
  static_assert(integral_c<i64 const&>);
  static_assert(integral_c<i64 const&&>);
  static_assert(integral_c<i64 volatile&>);
  static_assert(integral_c<i64 volatile&&>);
  static_assert(integral_c<i64 const volatile&>);
  static_assert(integral_c<i64 const volatile&&>);

  static_assert(float_c<f64>);
  static_assert(float_c<f64&>);
  static_assert(float_c<f64&&>);
  static_assert(float_c<f64 const>);
  static_assert(float_c<f64 volatile>);
  static_assert(float_c<f64 const&>);
  static_assert(float_c<f64 const&&>);
  static_assert(float_c<f64 volatile&>);
  static_assert(float_c<f64 volatile&&>);
  static_assert(float_c<f64 const volatile&>);
  static_assert(float_c<f64 const volatile&&>);

  static_assert(arithmetic_c<u64>);
  static_assert(arithmetic_c<u64&>);
  static_assert(arithmetic_c<u64&&>);
  static_assert(arithmetic_c<u64 const>);
  static_assert(arithmetic_c<u64 volatile>);
  static_assert(arithmetic_c<u64 const&>);
  static_assert(arithmetic_c<u64 const&&>);
  static_assert(arithmetic_c<u64 volatile&>);
  static_assert(arithmetic_c<u64 volatile&&>);
  static_assert(arithmetic_c<u64 const volatile&>);
  static_assert(arithmetic_c<u64 const volatile&&>);

  static_assert(arithmetic_c<f64>);
  static_assert(arithmetic_c<f64&>);
  static_assert(arithmetic_c<f64&&>);
  static_assert(arithmetic_c<f64 const>);
  static_assert(arithmetic_c<f64 volatile>);
  static_assert(arithmetic_c<f64 const&>);
  static_assert(arithmetic_c<f64 const&&>);
  static_assert(arithmetic_c<f64 volatile&>);
  static_assert(arithmetic_c<f64 volatile&&>);
  static_assert(arithmetic_c<f64 const volatile&>);
  static_assert(arithmetic_c<f64 const volatile&&>);

  static_assert(!f32_c<f64>);
  static_assert(!f64_c<f32>);
  static_assert(!i32_c<i64>);
  static_assert(!i64_c<i32>);
  static_assert(!u32_c<u64>);
  static_assert(!u64_c<u32>);

  static_assert(!integral_c<f64>);
  static_assert(!integral_c<f32>);
  static_assert(!float_c<i64>);
  static_assert(!float_c<i32>);
  static_assert(!float_c<u64>);
  static_assert(!float_c<u32>);
}
