#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace kaspan {
namespace index_map_avx2 {
#ifdef __AVX2__

/// loads a key into an avx2 register as [key, key, key, ...]
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
[[nodiscard]] inline auto
avx2_load_key(
  T key) noexcept -> __m256i
{
  if constexpr (u32_concept<T>) return _mm256_set1_epi32(std::bit_cast<i32>(key));
  else return _mm256_set1_epi64x(std::bit_cast<i64>(key));
}

/// loads the keys of a cacheline into an avx2 register as [key[0], key[1], key[2], ...]
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
[[nodiscard]] inline auto
avx2_load_keys(
  T const* keys) noexcept -> __m256i
{
  // this is safe only because the memory is cache line aligned
  auto const raw_keys = static_cast<void const*>(keys);
  return _mm256_load_si256(static_cast<__m256i const*>(raw_keys));
}

/// compares two avx2 registers and returns a mask
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
[[nodiscard]] inline auto
avx2_cmp_keys(
  __m256i avx2_key,
  __m256i avx2_keys) noexcept -> int
{
  if (u32_concept<T>) return _mm256_movemask_epi8(_mm256_cmpeq_epi32(avx2_keys, avx2_key));
  return _mm256_movemask_epi8(_mm256_cmpeq_epi64(avx2_keys, avx2_key));
}

/// for a non zero mask returns the first bit index
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
[[nodiscard]] inline auto
avx2_get_loc(
  int avx2_mask) noexcept
{
  return std::countr_zero(static_cast<u32>(avx2_mask)) / sizeof(T);
}

#endif
}

/// special hashmap that maps global indices to continuous local ones.
/// only inserted keys are allowed to be queried.
/// capacity must at least be half the number of elements to store (100% oppacity).
/// further insertions are undefined behaviour (infinite loop).
/// uses linear probing so the default capacity is twice the size requested (hard-coded).
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
class hash_index_map_view
{
  static constexpr u8 line_values = u32_concept<T> ? 16 : 8;
  static constexpr u8 line_pairs  = u32_concept<T> ? 8 : 4;

  T* m_data = nullptr;
  T  m_mask = 0;

public:
  hash_index_map_view() = default;

  hash_index_map_view(
    T* data,
    T  mask)
    : m_data(data)
    , m_mask(mask)
  {
  }

  ~hash_index_map_view() noexcept = default;

  hash_index_map_view(hash_index_map_view const&)         = default;
  hash_index_map_view(hash_index_map_view&& rhs) noexcept = default;

  hash_index_map_view& operator=(hash_index_map_view const&) = default;
  hash_index_map_view& operator=(hash_index_map_view&& rhs)  = default;

  [[nodiscard]] inline auto get(
    T key) const noexcept -> T
  {
    auto cache_line = hash(key) & m_mask;
    IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
    do {

      T* keys = m_data + cache_line * line_values;
      T* vals = keys + line_pairs;

#ifdef __AVX2__
      using namespace index_map_avx2;
      auto const avx2_key  = avx2_load_key(key);
      auto const avx2_keys = avx2_load_keys(keys);
      if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys)) {
        return vals[avx2_get_loc<T>(avx2_mask)];
      }
#else
      for (u32 i = 0; i < line_pairs; ++i) {
        if (keys[i] == key) return vals[i];
      }
#endif

      cache_line = cache_line + 1 & m_mask;
      DEBUG_ASSERT_NE(cache_line, first_cache_line, "key not in map (undefined behaviour)");
    } while (true);
  }
};

template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
class hash_index_map
{
  static constexpr T  null_key    = u32_concept<T> ? 0xffffffff : 0xffffffffffffffffull;
  static constexpr u8 line_bytes  = 64;
  static constexpr u8 pair_bytes  = u32_concept<T> ? 8 : 16;
  static constexpr u8 line_values = u32_concept<T> ? 16 : 8;
  static constexpr u8 line_pairs  = u32_concept<T> ? 8 : 4;

  T* m_data  = nullptr;
  T  m_mask  = 0;
  T  m_count = 0;

public:
  hash_index_map() = default;

  explicit hash_index_map(
    arithmetic_concept auto size)
  {
    if (size) {
      auto const cache_lines = std::bit_ceil(ceildiv<line_bytes>(integral_cast<u64>(size) * 2 * pair_bytes));
      auto const alloc_bytes = cache_lines * line_bytes;

      m_data = static_cast<T*>(line_alloc(alloc_bytes));
      std::memset(m_data, 0xff, alloc_bytes);
      m_mask  = integral_cast<T>(cache_lines - 1);
      m_count = 0;
    }
  }

  ~hash_index_map()
  {
    line_free(m_data);
  }

  hash_index_map(hash_index_map const&) = delete;
  hash_index_map(
    hash_index_map&& rhs) noexcept
    : m_data(rhs.m_data)
    , m_mask(rhs.m_mask)
    , m_count(rhs.m_count)
  {
    rhs.m_data  = nullptr;
    rhs.m_mask  = 0;
    rhs.m_count = 0;
  }

  hash_index_map& operator=(hash_index_map const&) = delete;
  hash_index_map& operator=(
    hash_index_map&& rhs) noexcept
  {
    if (this != &rhs) {
      line_free(m_data);
      m_data      = rhs.m_data;
      m_mask      = rhs.m_mask;
      m_count     = rhs.m_count;
      rhs.m_data  = nullptr;
      rhs.m_mask  = 0;
      rhs.m_count = 0;
    }
    return *this;
  }

  inline auto insert(
    T key) noexcept -> T
  {
    auto cache_line = hash(key) & m_mask;
    IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
    do {

      T* keys = m_data + cache_line * line_values;
      T* vals = keys + line_pairs;

#ifdef __AVX2__
      using namespace index_map_avx2;
      auto const avx2_key  = avx2_load_key(key);
      auto const avx2_keys = avx2_load_keys(keys);
      if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys)) {
        return vals[avx2_get_loc<T>(avx2_mask)];
      }
      auto const avx2_null = avx2_load_key(null_key);
      if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_null, avx2_keys)) {
        auto const i   = avx2_get_loc<T>(avx2_mask);
        auto const val = m_count++;
        keys[i]        = key;
        vals[i]        = val;
        return val;
      }
#else
      for (u32 i = 0; i < line_pairs; ++i) {
        if (keys[i] == key) return vals[i]; // already assigned
        if (keys[i] == null_key) {
          auto const val = m_count++;
          keys[i]        = key;
          vals[i]        = val;
          return val;
        }
      }
#endif

      cache_line = cache_line + 1 & m_mask;
      DEBUG_ASSERT_NE(cache_line, first_cache_line, "map is full (undefined behaviour)");
    } while (true);
  }

  [[nodiscard]] inline auto get(
    T key) const noexcept -> T
  {
    return view().get(key);
  }

  [[nodiscard]] inline auto view() const noexcept -> hash_index_map_view<T>
  {
    return { m_data, m_mask };
  }

  [[nodiscard]] inline auto count() const noexcept -> T
  {
    return m_count;
  }
};

} // namespace kaspan
