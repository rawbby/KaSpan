#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>
#include <optional>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace kaspan {

#ifdef __AVX2__
namespace hash_map_avx2 {

template<integral_c T>
[[nodiscard]] auto
avx2_set1_key(
  T key) noexcept -> __m256i
{
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
  auto const raw_keys = static_cast<void const*>(keys);
  return _mm256_load_si256(static_cast<__m256i const*>(raw_keys));
}

template<integral_c T>
[[nodiscard]] auto
avx2_cmpeq_keys(
  __m256i key,
  __m256i keys) noexcept
{
#if defined(__AVX512F__)
  if constexpr (sizeof(T) == 8) return _mm256_cmpeq_epi64_mask(keys, key);
  else if constexpr (sizeof(T) == 4) return _mm256_cmpeq_epi32_mask(keys, key);
  else if constexpr (sizeof(T) == 2) return _mm256_cmpeq_epi16_mask(keys, key);
  else return _mm256_cmpeq_epi8_mask(keys, key);
#else
  if constexpr (sizeof(T) == 8) return _mm256_movemask_epi8(_mm256_cmpeq_epi64(keys, key));
  else if constexpr (sizeof(T) == 4) return _mm256_movemask_epi8(_mm256_cmpeq_epi32(keys, key));
  else if constexpr (sizeof(T) == 2) return _mm256_movemask_epi8(_mm256_cmpeq_epi16(keys, key));
  else return _mm256_movemask_epi8(_mm256_cmpeq_epi8(keys, key));
#endif
}

template<integral_c T>
[[nodiscard]] auto
avx2_mask_first(
  auto mask) noexcept -> u32
{
#if defined(__AVX512F__)
  return std::countr_zero(static_cast<u32>(mask));
#else
  return std::countr_zero(static_cast<u32>(mask)) / sizeof(T);
#endif
}

template<integral_c T>
[[nodiscard]] auto
avx2_mask_count(
  auto mask) noexcept -> u32
{
#if defined(__AVX512F__)
  return std::popcount(static_cast<u32>(mask));
#else
  return std::popcount(static_cast<u32>(mask)) / sizeof(T);
#endif
}

}
#endif

template<integral_c T>
class hash_map
{
  static constexpr u8 line_bytes = 64;
  static constexpr u8 pair_bytes = 2 * sizeof(T);

  T* m_data = nullptr;
  T  m_mask = 0;

public:
  hash_map() = default;
  explicit hash_map(
    integral_c auto size)
  {
    if (size) {
      auto const cache_lines = std::bit_ceil(ceildiv<line_bytes>(integral_cast<u64>(size) * 2 * pair_bytes));
      auto const alloc_bytes = cache_lines * line_bytes;

      m_data = static_cast<T*>(line_alloc(alloc_bytes));
      m_mask = integral_cast<T>(cache_lines - 1);
      clear();
    }
  }

  ~hash_map()
  {
    line_free(m_data);
  }

  hash_map(hash_map const&) = delete;
  hash_map(
    hash_map&& rhs) noexcept
    : m_data(rhs.m_data)
    , m_mask(rhs.m_mask)
  {
    rhs.m_data = nullptr;
    rhs.m_mask = 0;
  }

  hash_map& operator=(hash_map const&) = delete;
  hash_map& operator=(
    hash_map&& rhs) noexcept
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

  auto find_null(
    T      key,
    auto&& on_null)
  {
#ifdef __AVX2__
    using namespace hash_map_avx2;
    auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

    auto cache_line = hash(key) & m_mask;
    IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
    for (;;) {
      T* keys = m_data + cache_line * 64 / sizeof(T);
      T* vals = keys + 32 / sizeof(T);
#ifdef __AVX2__
      auto const avx2_keys = avx2_load_keys(keys);
      if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx_null, avx2_keys)) {
        auto const i = avx2_mask_first<T>(avx2_mask);
        return on_null(keys[i], vals[i]);
      }
#else
      for (u8 i = 0; i < 32 / sizeof(T); ++i) {
        if (keys[i] == static_cast<T>(-1)) return on_null(keys[i], vals[i]);
      }
#endif
      cache_line = cache_line + 1 & m_mask;
      DEBUG_ASSERT_NE(cache_line, first_cache_line);
    }
  }

  auto find_key(
    T      key,
    auto&& on_key)
  {
#ifdef __AVX2__
    using namespace hash_map_avx2;
    auto const avx2_key = avx2_set1_key(key);
#endif

    auto cache_line = hash(key) & m_mask;
    IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
    for (;;) {
      T* keys = m_data + cache_line * 64 / sizeof(T);
      T* vals = keys + 32 / sizeof(T);
#ifdef __AVX2__
      auto const avx2_keys = avx2_load_keys(keys);
      if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_keys)) {
        auto const i = avx2_mask_first<T>(avx2_mask);
        return on_key(keys[i], vals[i]);
      }
#else
      for (u32 i = 0; i < 32 / sizeof(T); ++i) {
        if (keys[i] == key) {
          return on_key(keys[i], vals[i]);
        }
      }
#endif
      cache_line = cache_line + 1 & m_mask;
      DEBUG_ASSERT_NE(cache_line, first_cache_line);
    }
  }

  auto find_key_or_null(
    T      key,
    auto&& on_key,
    auto&& on_null)
  {
#ifdef __AVX2__
    using namespace hash_map_avx2;
    auto const avx2_key  = avx2_set1_key(key);
    auto const avx2_null = avx2_set1_key(static_cast<T>(-1));
#endif

    auto cache_line = hash(key) & m_mask;
    IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
    for (;;) {
      T* keys = m_data + cache_line * 64 / sizeof(T);
      T* vals = keys + 32 / sizeof(T);
#ifdef __AVX2__
      auto const avx2_keys = avx2_load_keys(keys);
      if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_keys)) {
        auto const i = avx2_mask_first<T>(avx2_mask);
        return on_key(keys[i], vals[i]);
      }
      if (auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_null, avx2_keys)) {
        auto const i = avx2_mask_first<T>(avx2_mask);
        return on_null(keys[i], vals[i]);
      }
#else
      for (u32 i = 0; i < 32 / sizeof(T); ++i) {
        if (keys[i] == key) return on_key(keys[i], vals[i]);
        if (keys[i] == static_cast<T>(-1)) return on_null(keys[i], vals[i]);
      }
#endif
      cache_line = cache_line + 1 & m_mask;
      DEBUG_ASSERT_NE(cache_line, first_cache_line);
    }
  }

  auto find_slot(
    T      key,
    auto&& on_slot)
  {
#ifdef __AVX2__
    using namespace hash_map_avx2;
    auto const avx2_key  = avx2_set1_key(key);
    auto const avx2_null = avx2_set1_key(static_cast<T>(-1));
#endif

    auto cache_line = hash(key) & m_mask;
    IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
    for (;;) {
      T* keys = m_data + cache_line * 64 / sizeof(T);
      T* vals = keys + 32 / sizeof(T);
#ifdef __AVX2__
      auto const avx2_keys = avx2_load_keys(keys);
      auto       avx2_mask = avx2_cmpeq_keys<T>(avx2_key, avx2_keys);
      avx2_mask |= avx2_cmpeq_keys<T>(avx2_null, avx2_keys);
      if (avx2_mask) {
        auto const i = avx2_mask_first<T>(avx2_mask);
        return on_slot(keys[i], vals[i]);
      }
#else
      for (u32 i = 0; i < 32 / sizeof(T); ++i) {
        if (keys[i] == key) return on_slot(keys[i], vals[i]);
        if (keys[i] == static_cast<T>(-1)) return on_slot(keys[i], vals[i]);
      }
#endif
      cache_line = cache_line + 1 & m_mask;
      DEBUG_ASSERT_NE(cache_line, first_cache_line);
    }
  }

  /// inserts a key value pair or assigns to an existing key
  void insert_or_assign(
    T key,
    T val) noexcept
  {
    find_slot(key, [&](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    });
  }

  /// write the value of an existing key
  void insert(
    T key,
    T val) noexcept
  {
    find_null(key, [&](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    });
  }

  /// write the value of an existing key
  void try_insert(
    T key,
    T val) noexcept
  {
    auto const on_key  = [](auto /* key_ */, auto /* val_ */) {};
    auto const on_null = [=](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    };
    find_key_or_null(key, on_key, on_null);
  }

  /// write the value of an existing key
  void try_insert(
    T      key,
    auto&& val_producer) noexcept
  {
    auto const on_key  = [](auto /* key_ */, auto /* val_ */) {};
    auto const on_null = [&](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val_producer();
    };
    find_key_or_null(key, on_key, on_null);
  }

  /// write the value of an existing key
  void assign(
    T key,
    T val) noexcept
  {
    find_key(key, [=](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    });
  }

  /// finds the key and swaps the value
  auto swap(
    T key,
    T val) noexcept -> T
  {
    return find_key(key, [=](auto& key_slot, auto& val_slot) {
      auto const val_ = val_slot;
      key_slot        = key;
      val_slot        = val;
      return val_;
    });
  }

  /// finds the key if inserted and swaps the value else insert
  auto swap_or_insert(
    T key,
    T val) noexcept -> std::optional<T>
  {
    auto const on_key = [=](auto& key_slot, auto& val_slot) -> std::optional<T> {
      auto const val_ = val_slot;
      key_slot        = key;
      val_slot        = val;
      return val_;
    };
    auto const on_null = [=](auto& key_slot, auto& val_slot) -> std::optional<T> {
      key_slot = key;
      val_slot = val;
      return std::nullopt;
    };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get(
    T key) noexcept -> T
  {
    return find_key(key, [](auto /* key_ */, auto val_) { return val_; });
  }

  [[nodiscard]] auto get_default(
    T key,
    T val) noexcept -> T
  {
    auto const on_key  = [](auto /* key_ */, auto val_) -> T { return val_; };
    auto const on_null = [=](auto /* key_ */, auto /* val_ */) -> T { return val; };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get_optional(
    T key) noexcept -> std::optional<T>
  {
    auto const on_key  = [](auto /* key_ */, auto val_) -> std::optional<T> { return val_; };
    auto const on_null = [](auto /* key_ */, auto /* val_ */) -> std::optional<T> { return std::nullopt; };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get_or_insert(
    T      key,
    auto&& val_producer) noexcept -> T
  {
    auto const on_key  = [](auto /* key_ */, auto val_) -> T { return val_; };
    auto const on_null = [=](auto& key_slot, auto& val_slot) -> T {
      auto const val = val_producer();
      key_slot       = key;
      val_slot       = val;
      return val;
    };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get_or_insert(
    T key,
    T val) noexcept -> T
  {
    auto const on_key  = [](auto /* key_ */, auto val_) -> T { return val_; };
    auto const on_null = [=](auto& key_slot, auto& val_slot) -> T {
      key_slot = key;
      val_slot = val;
      return val;
    };
    return find_key_or_null(key, on_key, on_null);
  }

  auto contains(
    T key) noexcept -> bool
  {
    auto const on_key  = [](auto /* key_ */, auto /* val_ */) -> bool { return true; };
    auto const on_null = [](auto /* key_ */, auto /* val_ */) -> bool { return false; };
    return find_key_or_null(key, on_key, on_null);
  }

  void clear() noexcept
  {
    if (m_data) std::memset(m_data, 0xff, (integral_cast<u64>(m_mask) + 1) * line_bytes);
  }

  [[nodiscard]] auto count() noexcept -> T
  {
#ifdef __AVX2__
    using namespace hash_map_avx2;
    auto const avx2_null = avx2_set1_key<T>(static_cast<T>(-1));
#endif

    T c = 0;

    auto const end = static_cast<u64>(m_mask) + 1;
    for (u64 cache_line = 0; cache_line < end; ++cache_line) {
      T* keys = m_data + cache_line * 64 / sizeof(T);
#ifdef __AVX2__
      auto const avx2_keys = avx2_load_keys(keys);
      auto const avx2_mask = avx2_cmpeq_keys<T>(avx2_null, avx2_keys);
      c += 32 / sizeof(T) - avx2_mask_count<T>(avx2_mask);
#else
      for (u8 i = 0; i < 32 / sizeof(T); ++i) {
        if (keys[i] != static_cast<T>(-1)) ++c;
      }
#endif
    }

    return c;
  }
};

} // namespace kaspan
