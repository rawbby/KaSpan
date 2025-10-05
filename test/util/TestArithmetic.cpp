#include <util/Arithmetic.hpp>

int
main()
{
  static_assert(I8Concept<char>);
  static_assert(I8Concept<signed char>);
  static_assert(U8Concept<unsigned char>);

  static_assert(I16Concept<short>);
  static_assert(I16Concept<signed short>);
  static_assert(U16Concept<unsigned short>);

  static_assert(I32Concept<int>);
  static_assert(I32Concept<signed int>);
  static_assert(U32Concept<unsigned int>);

  static_assert(I64Concept<long long>);
  static_assert(I64Concept<signed long long>);
  static_assert(U64Concept<unsigned long long>);

  static_assert(F32Concept<float>);
  static_assert(F64Concept<double>);

  static_assert(ByteConcept<byte>);
  static_assert(not I8Concept<byte>);
  static_assert(not U8Concept<byte>);
  static_assert(I8Concept<std::int8_t>);
  static_assert(U8Concept<std::uint8_t>);
  static_assert(I16Concept<std::int16_t>);
  static_assert(U16Concept<std::uint16_t>);
  static_assert(I32Concept<std::int32_t>);
  static_assert(U32Concept<std::uint32_t>);
  static_assert(I64Concept<std::int64_t>);
  static_assert(U64Concept<std::uint64_t>);

  static_assert(not has_uint128_t or U128Concept<u128>);
  static_assert(not has_int128_t or I128Concept<i128>);

  static_assert(std::same_as<byte, byte>);
  static_assert(std::same_as<u8, std::uint8_t>);
  static_assert(std::same_as<u16, std::uint16_t>);
  static_assert(std::same_as<u32, std::uint32_t>);
  static_assert(std::same_as<u64, std::uint64_t>);
  static_assert(std::same_as<i8, std::int8_t>);
  static_assert(std::same_as<i16, std::int16_t>);
  static_assert(std::same_as<i32, std::int32_t>);
  static_assert(std::same_as<i64, std::int64_t>);

  static_assert(ByteConcept<byte>);
  static_assert(U8Concept<u8>);
  static_assert(U16Concept<u16>);
  static_assert(U32Concept<u32>);
  static_assert(U64Concept<u64>);
  static_assert(I8Concept<i8>);
  static_assert(I16Concept<i16>);
  static_assert(I32Concept<i32>);
  static_assert(I64Concept<i64>);

  static_assert(FloatConcept<f32>);
  static_assert(FloatConcept<f64>);

  static_assert(not UnsignedConcept<byte>);
  static_assert(UnsignedConcept<u8>);
  static_assert(UnsignedConcept<u16>);
  static_assert(UnsignedConcept<u32>);
  static_assert(UnsignedConcept<u64>);

  static_assert(SignedConcept<i8>);
  static_assert(SignedConcept<i16>);
  static_assert(SignedConcept<i32>);
  static_assert(SignedConcept<i64>);

  static_assert(not IntConcept<byte>);
  static_assert(IntConcept<u8>);
  static_assert(IntConcept<u16>);
  static_assert(IntConcept<u32>);
  static_assert(IntConcept<u64>);
  static_assert(IntConcept<i8>);
  static_assert(IntConcept<i16>);
  static_assert(IntConcept<i32>);
  static_assert(IntConcept<i64>);

  static_assert(not ArithmeticConcept<byte>);
  static_assert(ArithmeticConcept<u8>);
  static_assert(ArithmeticConcept<u16>);
  static_assert(ArithmeticConcept<u32>);
  static_assert(ArithmeticConcept<u64>);
  static_assert(ArithmeticConcept<i8>);
  static_assert(ArithmeticConcept<i16>);
  static_assert(ArithmeticConcept<i32>);
  static_assert(ArithmeticConcept<i64>);
  static_assert(ArithmeticConcept<f32>);
  static_assert(ArithmeticConcept<f64>);

  static_assert(has_uint128_t == U128Concept<u128>);
  static_assert(has_int128_t == I128Concept<i128>);

  static_assert(has_uint128_t or std::is_same_v<u128, std::monostate>);
  static_assert(has_int128_t or std::is_same_v<i128, std::monostate>);

  static_assert(U32Concept<u32>);
  static_assert(U32Concept<u32&>);
  static_assert(U32Concept<u32&&>);
  static_assert(U32Concept<u32 const>);
  static_assert(U32Concept<u32 volatile>);
  static_assert(U32Concept<u32 const&>);
  static_assert(U32Concept<u32 const&&>);
  static_assert(U32Concept<u32 volatile&>);
  static_assert(U32Concept<u32 volatile&&>);
  static_assert(U32Concept<u32 const volatile&>);
  static_assert(U32Concept<u32 const volatile&&>);

  static_assert(I32Concept<i32>);
  static_assert(I32Concept<i32&>);
  static_assert(I32Concept<i32&&>);
  static_assert(I32Concept<i32 const>);
  static_assert(I32Concept<i32 volatile>);
  static_assert(I32Concept<i32 const&>);
  static_assert(I32Concept<i32 const&&>);
  static_assert(I32Concept<i32 volatile&>);
  static_assert(I32Concept<i32 volatile&&>);
  static_assert(I32Concept<i32 const volatile&>);
  static_assert(I32Concept<i32 const volatile&&>);

  static_assert(F32Concept<f32>);
  static_assert(F32Concept<f32&>);
  static_assert(F32Concept<f32&&>);
  static_assert(F32Concept<f32 const>);
  static_assert(F32Concept<f32 volatile>);
  static_assert(F32Concept<f32 const&>);
  static_assert(F32Concept<f32 const&&>);
  static_assert(F32Concept<f32 volatile&>);
  static_assert(F32Concept<f32 volatile&&>);
  static_assert(F32Concept<f32 const volatile&>);
  static_assert(F32Concept<f32 const volatile&&>);

  static_assert(IntConcept<i64>);
  static_assert(IntConcept<i64&>);
  static_assert(IntConcept<i64&&>);
  static_assert(IntConcept<i64 const>);
  static_assert(IntConcept<i64 volatile>);
  static_assert(IntConcept<i64 const&>);
  static_assert(IntConcept<i64 const&&>);
  static_assert(IntConcept<i64 volatile&>);
  static_assert(IntConcept<i64 volatile&&>);
  static_assert(IntConcept<i64 const volatile&>);
  static_assert(IntConcept<i64 const volatile&&>);

  static_assert(FloatConcept<f64>);
  static_assert(FloatConcept<f64&>);
  static_assert(FloatConcept<f64&&>);
  static_assert(FloatConcept<f64 const>);
  static_assert(FloatConcept<f64 volatile>);
  static_assert(FloatConcept<f64 const&>);
  static_assert(FloatConcept<f64 const&&>);
  static_assert(FloatConcept<f64 volatile&>);
  static_assert(FloatConcept<f64 volatile&&>);
  static_assert(FloatConcept<f64 const volatile&>);
  static_assert(FloatConcept<f64 const volatile&&>);

  static_assert(ArithmeticConcept<u64>);
  static_assert(ArithmeticConcept<u64&>);
  static_assert(ArithmeticConcept<u64&&>);
  static_assert(ArithmeticConcept<u64 const>);
  static_assert(ArithmeticConcept<u64 volatile>);
  static_assert(ArithmeticConcept<u64 const&>);
  static_assert(ArithmeticConcept<u64 const&&>);
  static_assert(ArithmeticConcept<u64 volatile&>);
  static_assert(ArithmeticConcept<u64 volatile&&>);
  static_assert(ArithmeticConcept<u64 const volatile&>);
  static_assert(ArithmeticConcept<u64 const volatile&&>);

  static_assert(ArithmeticConcept<f64>);
  static_assert(ArithmeticConcept<f64&>);
  static_assert(ArithmeticConcept<f64&&>);
  static_assert(ArithmeticConcept<f64 const>);
  static_assert(ArithmeticConcept<f64 volatile>);
  static_assert(ArithmeticConcept<f64 const&>);
  static_assert(ArithmeticConcept<f64 const&&>);
  static_assert(ArithmeticConcept<f64 volatile&>);
  static_assert(ArithmeticConcept<f64 volatile&&>);
  static_assert(ArithmeticConcept<f64 const volatile&>);
  static_assert(ArithmeticConcept<f64 const volatile&&>);

  static_assert(not F32Concept<f64>);
  static_assert(not F64Concept<f32>);
  static_assert(not I32Concept<i64>);
  static_assert(not I64Concept<i32>);
  static_assert(not U32Concept<u64>);
  static_assert(not U64Concept<u32>);

  static_assert(not IntConcept<f64>);
  static_assert(not IntConcept<f32>);
  static_assert(not FloatConcept<i64>);
  static_assert(not FloatConcept<i32>);
  static_assert(not FloatConcept<u64>);
  static_assert(not FloatConcept<u32>);
}
