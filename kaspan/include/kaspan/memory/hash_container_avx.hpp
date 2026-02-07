#pragma once

#include <kaspan/util/arithmetic.hpp>

#include <bit>

#if defined(__AVX512F__) || defined(__AVX2__)
#include <immintrin.h>
#endif

namespace kaspan {

#if defined(__AVX512F__) || defined(__AVX2__)

template<integral_c T>
[[nodiscard]] auto
avx2_set1_key(
  T key) noexcept -> __m256i
{
  // note: _mm256_set1_epi is def for both AVX2 and AVX512
  if constexpr (sizeof(T) == 8) return _mm256_set1_epi64(std::bit_cast<i64>(key));
  else if constexpr (sizeof(T) == 4) return _mm256_set1_epi32(std::bit_cast<i32>(key));
  else if constexpr (sizeof(T) == 2) return _mm256_set1_epi16(std::bit_cast<i16>(key));
  else return _mm256_set1_epi8(std::bit_cast<i8>(key));
}

template<integral_c T>
[[nodiscard]] auto
avx2_load_keys(
  T const* keys) noexcept -> __m256i
{
  // note: _mm256_load_si256 is def for both AVX2 and AVX512
  return _mm256_load_si256(static_cast<__m256i const*>(static_cast<void const*>(keys)));
}

template<integral_c T>
[[nodiscard]] auto
avx2_cmpeq_keys(
  __m256i key,
  __m256i keys) noexcept
{
  // note: prefer AVX512 mask over AVX2 mask
#if defined(__AVX512F__)
  if constexpr (sizeof(T) == 8) return _mm256_cmpeq_epi64_mask(keys, key);
  else if constexpr (sizeof(T) == 4) return _mm256_cmpeq_epi32_mask(keys, key);
  else if constexpr (sizeof(T) == 2) return _mm256_cmpeq_epi16_mask(keys, key);
  else return _mm256_cmpeq_epi8_mask(keys, key);
#else
  if constexpr (sizeof(T) == 8) return static_cast<u32>(_mm256_movemask_epi8(_mm256_cmpeq_epi64(keys, key)));
  else if constexpr (sizeof(T) == 4) return static_cast<u32>(_mm256_movemask_epi8(_mm256_cmpeq_epi32(keys, key)));
  else if constexpr (sizeof(T) == 2) return static_cast<u32>(_mm256_movemask_epi8(_mm256_cmpeq_epi16(keys, key)));
  else return static_cast<u32>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(keys, key)));
#endif
}

#if !defined(__AVX512F__)
[[nodiscard]] inline auto
avx2_mask_combine(
  u32 mask_lo,
  u32 mask_hi) noexcept -> u64
{
  // note: match the mask choosen from avx2_cmpeq_keys
  return static_cast<u64>(mask_hi) << 32 | mask_lo;
}
#endif

template<integral_c T>
[[nodiscard]] auto
avx2_mask_first(
  auto mask) noexcept -> int
{
  // note: match the mask choosen from avx2_cmpeq_keys
#if defined(__AVX512F__)
  return std::countr_zero(mask);
#else
  return std::countr_zero(mask) / sizeof(T);
#endif
}

template<integral_c T>
[[nodiscard]] auto
avx2_mask_count(
  auto mask) noexcept -> int
{
  // note: match the mask choosen from avx2_cmpeq_keys
#if defined(__AVX512F__)
  return std::popcount(mask);
#else
  return std::popcount(mask) / sizeof(T);
#endif
}

#if defined(__AVX512F__)

template<integral_c T>
[[nodiscard]] auto
avx512_set1_key(
  T key) noexcept -> __m512i
{
  if constexpr (sizeof(T) == 8) return _mm512_set1_epi64(std::bit_cast<i64>(key));
  else if constexpr (sizeof(T) == 4) return _mm512_set1_epi32(std::bit_cast<i32>(key));
  else if constexpr (sizeof(T) == 2) return _mm512_set1_epi16(std::bit_cast<i16>(key));
  else return _mm512_set1_epi8(std::bit_cast<i8>(key));
}

[[nodiscard]] inline auto
avx512_load_keys(
  void const* keys) noexcept -> __m512i
{
  return _mm512_load_si512(keys);
}

template<integral_c T>
[[nodiscard]] auto
avx512_cmpeq_keys(
  __m512i key,
  __m512i keys) noexcept
{
  if constexpr (sizeof(T) == 8) return _mm512_cmpeq_epi64_mask(keys, key);
  else if constexpr (sizeof(T) == 4) return _mm512_cmpeq_epi32_mask(keys, key);
  else if constexpr (sizeof(T) == 2) return _mm512_cmpeq_epi16_mask(keys, key);
  else return _mm512_cmpeq_epi8_mask(keys, key);
}

[[nodiscard]] auto
avx512_mask_first(
  auto mask) noexcept -> int
{
  return std::countr_zero(mask);
}

[[nodiscard]] auto
avx512_mask_count(
  auto mask) noexcept -> int
{
  return std::popcount(mask);
}

template<integral_c T>
[[nodiscard]] auto
avx_mask_lor(
  T mask_a,
  T mask_b) noexcept -> T
{
  return mask_a | mask_b;
}

#endif
#endif

}
