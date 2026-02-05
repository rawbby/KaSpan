#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>
#include <optional>

#undef __AVX512F__

#if defined(__AVX512F__) || defined(__AVX2__)
#include <immintrin.h>
#endif

namespace kaspan::hash_map_util {

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
  void const* keys) noexcept -> __m256i
{
  // note: _mm256_load_si256 is def for both AVX2 and AVX512
  return _mm256_load_si256(static_cast<__m256i const*>(keys));
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

#endif
#endif

template<integral_c T>
auto
find256_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_null)
{
#if defined(__AVX2__)
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx_null, avx2_load_keys(keys))) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_null(keys[i], vals[i]);
    }
#else
    for (u8 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == static_cast<T>(-1)) return on_null(keys[i], vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find256_key(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key)
{
#if defined(__AVX2__)
  auto const avx2_key = avx2_set1_key(key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_load_keys(keys))) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_key(vals[i]);
    }
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == key) return on_key(vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find256_key_or_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key,
  auto&& on_null)
{
#if defined(__AVX2__)
  auto const avx2_key  = avx2_set1_key(key);
  auto const avx2_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_keys)) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_key(vals[i]);
    }
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_null(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == key) return on_key(vals[i]);
      if (keys[i] == static_cast<T>(-1)) return on_null(keys[i], vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find256_slot(
  T*     data,
  T      mask,
  T      key,
  auto&& on_slot)
{
#if defined(__AVX2__)
  auto const avx2_key  = avx2_set1_key(key);
  auto const avx2_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_keys) | avx2_cmpeq_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_slot(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == key) return on_slot(keys[i], vals[i]);
      if (keys[i] == static_cast<T>(-1)) return on_slot(keys[i], vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find512_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_null)
{
#if defined(__AVX2__)
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx_null, avx2_load_keys(keys))) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_null(keys[i], vals[i]);
    }
#else
    for (u8 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == static_cast<T>(-1)) return on_null(keys[i], vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find512_key(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key)
{
#if defined(__AVX2__)
  auto const avx2_key = avx2_set1_key(key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_load_keys(keys))) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_key(vals[i]);
    }
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == key) return on_key(vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find512_key_or_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key,
  auto&& on_null)
{
#if defined(__AVX2__)
  auto const avx2_key  = avx2_set1_key(key);
  auto const avx2_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_keys)) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_key(vals[i]);
    }
    if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_null(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == key) return on_key(vals[i]);
      if (keys[i] == static_cast<T>(-1)) return on_null(keys[i], vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
find512_slot(
  T*     data,
  T      mask,
  T      key,
  auto&& on_slot)
{
#if defined(__AVX512F__)
  auto const avx512_key  = avx512_set1_key(key);
  auto const avx512_null = avx512_set1_key(static_cast<T>(-1));
#elif defined(__AVX2__)
  auto const avx2_key  = avx2_set1_key(key);
  auto const avx2_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);

#if defined(__AVX512F__)
    auto const avx512_keys = avx512_load_keys(keys);
    if (auto const avx512_mask = avx512_cmpeq_keys<T>(avx512_key, avx512_keys) | avx512_cmpeq_keys<T>(avx512_null, avx512_keys)) {
      auto const i = avx512_mask_first<T>(avx512_mask);
      return on_slot(keys[i]);
    }
#elif defined(__AVX2__)
    auto const avx2_keys_lo = avx2_load_keys(keys);
    auto const avx2_keys_hi = avx2_load_keys(keys + 32 / sizeof(T));
    auto const avx2_mask_lo = avx2_cmpeq_keys<T>(avx2_key, avx2_keys_lo) | avx2_cmpeq_keys<T>(avx2_null, avx2_keys_lo);
    auto const avx2_mask_hi = avx2_cmpeq_keys<T>(avx2_key, avx2_keys_hi) | avx2_cmpeq_keys<T>(avx2_null, avx2_keys_hi);
    if (auto const avx2_mask = avx2_mask_combine(avx2_mask_lo, avx2_mask_hi)) {
      auto const i = avx2_mask_first<T>(avx2_mask);
      return on_slot(keys[i]);
    }
#else
    for (u32 i = 0; i < 64 / sizeof(T); ++i) {
      if (keys[i] == key) return on_slot(keys[i]);
      if (keys[i] == static_cast<T>(-1)) return on_slot(keys[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
void
each_line(
  T*     m_data,
  T      m_mask,
  auto&& on_line) noexcept
{
  auto const end = static_cast<u64>(m_mask) + 1;
  for (u64 it = 0; it < end; ++it) {
    on_line(m_data + it * 64 / sizeof(T));
  }
}

}
