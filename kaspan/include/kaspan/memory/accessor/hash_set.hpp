#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>

#if defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

namespace kaspan {
namespace hash_set_internal {

// #ifdef __AVX512F__

// /// loads a key into an avx512 register as [key, key, key, ...]
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx512_load_key(
//   T key) noexcept -> __m512i
// {
//   if constexpr (u32_c<T>) return _mm512_set1_epi32(std::bit_cast<i32>(key));
//   else return _mm512_set1_epi64x(std::bit_cast<i64>(key));
// }

// /// loads the keys of a cacheline into an avx512 register as [key[0], key[1], key[2], ...]
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx512_load_keys(
//   T const* keys) noexcept -> __m512i
// {
//   // this is safe only because the memory is cache line aligned
//   return _mm512_load_si512(static_cast<void const*>(keys));
// }

// /// compares two avx512 registers and returns a mask
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx512_cmp_keys(
//   __m512i avx512_key,
//   __m512i avx512_keys) noexcept
// {
//   if constexpr (u32_c<T>) return _mm512_cmpeq_epi32_mask(avx512_keys, avx512_key);
//   else return _mm512_cmpeq_epi64_mask(avx512_keys, avx512_key);
// }

// /// for a non zero mask returns the first bit index
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx512_get_loc(
//   auto avx512_mask) noexcept
// {
//   return std::countr_zero(static_cast<u32>(avx512_mask));
// }

// #endif

// #ifdef __AVX2__

// /// loads a key into an avx2 register as [key, key, key, ...]
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx2_load_key(
//   T key) noexcept -> __m256i
// {
//   if constexpr (u32_c<T>) return _mm256_set1_epi32(std::bit_cast<i32>(key));
//   else return _mm256_set1_epi64x(std::bit_cast<i64>(key));
// }

// /// loads the keys of a cacheline into an avx2 register as [key[0], key[1], key[2], ...]
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx2_load_keys(
//   T const* keys) noexcept -> __m256i
// {
//   // this is safe only because the memory is cache line aligned
//   auto const raw_keys = static_cast<void const*>(keys);
//   return _mm256_load_si256(static_cast<__m256i const*>(raw_keys));
// }

// /// compares two avx2 registers and returns a mask
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx2_cmp_keys(
//   __m256i avx2_key,
//   __m256i avx2_keys) noexcept -> int
// {
//   if (u32_c<T>) return _mm256_movemask_epi8(_mm256_cmpeq_epi32(avx2_keys, avx2_key));
//   return _mm256_movemask_epi8(_mm256_cmpeq_epi64(avx2_keys, avx2_key));
// }

// /// for a non zero mask returns the first bit index
// template<typename T>
//   requires(u32_c<T> || u64_c<T>)
// [[nodiscard]] auto
// avx2_get_loc(
//   int avx2_mask) noexcept
// {
//   return std::countr_zero(static_cast<u32>(avx2_mask)) / sizeof(T);
// }

// #endif

template<integral_c T>
auto
null_key() noexcept
{
  // clang-format off
#if defined(__AVX512F__)
       if constexpr (sizeof(T) == 1) return _mm512_set1_epi8(static_cast<T>(-1));
  else if constexpr (sizeof(T) == 2) return _mm512_set1_epi16(static_cast<T>(-1));
  else if constexpr (sizeof(T) == 4) return _mm512_set1_epi32(static_cast<T>(-1));
  else if constexpr (sizeof(T) == 8) return _mm512_set1_epi64(static_cast<T>(-1));
  else
#endif
#if defined(__AVX2__)
         if constexpr (sizeof(T) == 1) return _mm256_set1_epi8(static_cast<T>(-1));
    else if constexpr (sizeof(T) == 2) return _mm256_set1_epi16(static_cast<T>(-1));
    else if constexpr (sizeof(T) == 4) return _mm256_set1_epi32(static_cast<T>(-1));
    else if constexpr (sizeof(T) == 8) return _mm256_set1_epi64(static_cast<T>(-1));
    else
#endif
  return static_cast<T>(-1);
  // clang-format on
}

template<typename T>
  requires(u32_c<T> || u64_c<T>)
auto
find_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_null)
{
  static constexpr T  null_key    = static_cast<T>(-1);
  static constexpr u8 line_values = 64 / sizeof(T);

#if defined(__AVX512F__)
  auto const avx_null = _mm512_set1_epi(static_cast<T>(-1));
#elif defined(__AVX2__)
  using namespace hash_set_internal;
  auto const avx2_null = avx2_load_key(null_key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
#ifdef __AVX512F__
    auto const avx512_keys = avx512_load_keys(keys);
    if (auto const avx512_mask = avx512_cmp_keys<T>(avx512_null, avx512_keys)) {
      auto const i = avx512_get_loc<T>(avx512_mask);
      return on_null(keys[i]);
    }
#elif defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_null(keys[i]);
    }
#else
    for (u8 i = 0; i < line_values; ++i) {
      if (keys[i] == null_key) return on_null(keys[i]);
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<typename T>
  requires(u32_c<T> || u64_c<T>)
auto
find_key(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key)
{
  static constexpr u8 line_values = 64 / sizeof(T);

#ifdef __AVX512F__
  auto const avx512_key = avx512_load_key(key);
#elif defined(__AVX2__)
  auto const avx2_key = avx2_load_key(key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
#ifdef __AVX512F__
    auto const avx512_keys = avx512_load_keys(keys);
    if (auto const avx512_mask = avx512_cmp_keys<T>(avx512_key, avx512_keys)) {
      auto const i = avx512_get_loc<T>(avx512_mask);
      return on_key(keys[i]);
    }
#elif defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_key(keys[i]);
    }
#else
    for (u32 i = 0; i < line_values; ++i) {
      if (keys[i] == key) {
        return on_key(keys[i]);
      }
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<typename T>
  requires(u32_c<T> || u64_c<T>)
auto
find_key_or_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key,
  auto&& on_null)
{
  static constexpr T  null_key    = static_cast<T>(-1);
  static constexpr u8 line_values = 64 / sizeof(T);

#ifdef __AVX512F__
  using namespace hash_set_internal;
  auto const avx512_key  = avx512_load_key(key);
  auto const avx512_null = avx512_load_key(null_key);
#elif defined(__AVX2__)
  using namespace hash_set_internal;
  auto const avx2_key  = avx2_load_key(key);
  auto const avx2_null = avx2_load_key(null_key);
#endif
  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
#ifdef __AVX512F__
    auto const avx512_keys = avx512_load_keys(keys);
    if (auto const avx512_mask = avx512_cmp_keys<T>(avx512_key, avx512_keys)) {
      auto const i = avx512_get_loc<T>(avx512_mask);
      return on_key(keys[i]);
    }
    if (auto const avx512_mask = avx512_cmp_keys<T>(avx512_null, avx512_keys)) {
      auto const i = avx512_get_loc<T>(avx512_mask);
      return on_null(keys[i]);
    }
#elif defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_key(keys[i]);
    }
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_null(keys[i]);
    }
#else
    for (u32 i = 0; i < line_values; ++i) {
      if (keys[i] == key) return on_key(keys[i]);
      if (keys[i] == null_key) return on_null(keys[i]);
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<typename T>
  requires(u32_c<T> || u64_c<T>)
auto
find_slot(
  T*     data,
  T      mask,
  T      key,
  auto&& on_slot)
{
  static constexpr T  null_key    = static_cast<T>(-1);
  static constexpr u8 line_values = 64 / sizeof(T);

#ifdef __AVX512F__
  using namespace hash_set_internal;
  auto const avx512_key  = avx512_load_key(key);
  auto const avx512_null = avx512_load_key(null_key);
#elif defined(__AVX2__)
  using namespace hash_set_internal;
  auto const avx2_key  = avx2_load_key(key);
  auto const avx2_null = avx2_load_key(null_key);
#endif
  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
#ifdef __AVX512F__
    auto const avx512_keys = avx512_load_keys(keys);
    auto       avx512_mask = avx512_cmp_keys<T>(avx512_key, avx512_keys);
    avx512_mask |= avx512_cmp_keys<T>(avx512_null, avx512_keys);
    if (avx512_mask) {
      auto const i = avx512_get_loc<T>(avx512_mask);
      return on_slot(keys[i]);
    }
#elif defined(__AVX2__)
    auto const avx2_keys = avx2_load_keys(keys);
    auto       avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys);
    avx2_mask |= avx2_cmp_keys<T>(avx2_null, avx2_keys);
    if (avx2_mask) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_slot(keys[i]);
    }
#else
    for (u32 i = 0; i < line_values; ++i) {
      if (keys[i] == key) return on_slot(keys[i]);
      if (keys[i] == null_key) return on_slot(keys[i]);
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}
}

template<typename T>
  requires(u32_c<T> || u64_c<T>)
class hash_set
{
  static constexpr u8 line_bytes = 64;

  T* m_data = nullptr;
  T  m_mask = 0;

public:
  hash_set() = default;
  explicit hash_set(
    integral_c auto size)
  {
    if (size) {
      auto const cache_lines = std::bit_ceil(ceildiv<line_bytes>(integral_cast<u64>(size) * 2 * sizeof(T)));
      auto const alloc_bytes = cache_lines * line_bytes;

      m_data = static_cast<T*>(line_alloc(alloc_bytes));
      m_mask = integral_cast<T>(cache_lines - 1);
      clear();
    }
  }

  ~hash_set()
  {
    line_free(m_data);
  }

  hash_set(hash_set const&) = delete;
  hash_set(
    hash_set&& rhs) noexcept
    : m_data(rhs.m_data)
    , m_mask(rhs.m_mask)
  {
    rhs.m_data = nullptr;
    rhs.m_mask = 0;
  }

  hash_set& operator=(hash_set const&) = delete;
  hash_set& operator=(
    hash_set&& rhs) noexcept
  {
    if (this != &rhs) {
      line_free(m_data);
      m_data     = rhs.m_data;
      m_mask     = rhs.m_mask;
      rhs.m_data = nullptr;
      rhs.m_mask = 0;
    }
    return *this;
  }

  /// inserts a key
  void insert(
    T key) noexcept
  {
    using namespace hash_set_internal;
    find_slot(m_data, m_mask, key, [&](auto& key_slot) { key_slot = key; });
  }

  /// returns true if the key was newly inserted
  auto try_insert(
    T key) noexcept -> bool
  {
    using namespace hash_set_internal;
    auto const on_key  = [](auto /* key_slot */) { return false; };
    auto const on_null = [=](auto& key_slot) {
      key_slot = key;
      return true;
    };
    return find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  auto contains(
    T key) const noexcept -> bool
  {
    using namespace hash_set_internal;
    auto const on_key  = [](auto /* key_slot */) -> bool { return true; };
    auto const on_null = [](auto /* key_slot */) -> bool { return false; };
    return find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  void clear() noexcept
  {
    if (m_data) std::memset(m_data, 0xff, (integral_cast<u64>(m_mask) + 1) * line_bytes);
  }

  [[nodiscard]] auto count() const noexcept -> T
  {
    static constexpr T  null_key    = static_cast<T>(-1);
    static constexpr u8 line_values = 64 / sizeof(T);

#ifdef __AVX512F__
    using namespace hash_set_internal;
    auto const avx512_null = avx512_load_key(null_key);
#elif defined(__AVX2__)
    using namespace hash_set_internal;
    auto const avx2_null = avx2_load_key(null_key);
#endif

    T c = 0;

    auto const end = static_cast<u64>(m_mask) + 1;
    for (u64 cache_line = 0; cache_line < end; ++cache_line) {
      T* keys = m_data + cache_line * line_values;
#ifdef __AVX512F__
      auto const avx512_mask = avx512_cmp_keys<T>(avx512_null, avx512_load_keys(keys));
      auto const null_count  = std::popcount(static_cast<u64>(avx512_mask));
      ASSERT_GE(line_values, null_count);
      c += line_values - null_count;
#elif defined(__AVX2__)
      auto const avx2_mask  = avx2_cmp_keys<T>(avx2_null, avx2_load_keys(keys));
      auto const null_count = std::popcount(static_cast<u32>(avx2_mask)) / sizeof(T);
      ASSERT_GE(line_values, null_count);
      c += line_values - null_count;
#else
      for (u8 i = 0; i < line_values; ++i) {
        if (keys[i] != null_key) ++c;
      }
#endif
    }

    return c;
  }
};

} // namespace kaspan
