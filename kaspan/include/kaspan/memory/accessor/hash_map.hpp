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
namespace hash_map_internal {
#ifdef __AVX2__

/// loads a key into an avx2 register as [key, key, key, ...]
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
[[nodiscard]] auto
avx2_load_key(
  T key) noexcept -> __m256i
{
  if constexpr (u32_concept<T>) return _mm256_set1_epi32(std::bit_cast<i32>(key));
  else return _mm256_set1_epi64x(std::bit_cast<i64>(key));
}

/// loads the keys of a cacheline into an avx2 register as [key[0], key[1], key[2], ...]
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
[[nodiscard]] auto
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
[[nodiscard]] auto
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
[[nodiscard]] auto
avx2_get_loc(
  int avx2_mask) noexcept
{
  return std::countr_zero(static_cast<u32>(avx2_mask)) / sizeof(T);
}

#endif

template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
auto
find_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_null)
{
  static constexpr T  null_key    = static_cast<T>(-1);
  static constexpr u8 line_values = 64 / sizeof(T);
  static constexpr u8 line_pairs  = line_values / 2;

#ifdef __AVX2__
  using namespace hash_map_internal;
  auto const avx2_null = avx2_load_key(null_key);
#endif
  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
    T* vals = keys + line_pairs;
#ifdef __AVX2__
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_null(keys[i], vals[i]);
    }
#else
    for (u8 i = 0; i < line_pairs; ++i) {
      if (keys[i] == null_key) return on_null(keys[i], vals[i]);
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
auto
find_key(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key)
{
  static constexpr u8 line_values = 64 / sizeof(T);
  static constexpr u8 line_pairs  = line_values / 2;

#ifdef __AVX2__
  auto const avx2_key = avx2_load_key(key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
    T* vals = keys + line_pairs;
#ifdef __AVX2__
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_key(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < line_pairs; ++i) {
      if (keys[i] == key) {
        return on_key(keys[i], vals[i]);
      }
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
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
  static constexpr u8 line_pairs  = line_values / 2;

#ifdef __AVX2__
  using namespace hash_map_internal;
  auto const avx2_key  = avx2_load_key(key);
  auto const avx2_null = avx2_load_key(null_key);
#endif
  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
    T* vals = keys + line_pairs;
#ifdef __AVX2__
    auto const avx2_keys = avx2_load_keys(keys);
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_key(keys[i], vals[i]);
    }
    if (auto const avx2_mask = avx2_cmp_keys<T>(avx2_null, avx2_keys)) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_null(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < line_pairs; ++i) {
      if (keys[i] == key) return on_key(keys[i], vals[i]);
      if (keys[i] == null_key) return on_null(keys[i], vals[i]);
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
auto
find_slot(
  T*     data,
  T      mask,
  T      key,
  auto&& on_slot)
{
  static constexpr T  null_key    = static_cast<T>(-1);
  static constexpr u8 line_values = 64 / sizeof(T);
  static constexpr u8 line_pairs  = line_values / 2;

#ifdef __AVX2__
  using namespace hash_map_internal;
  auto const avx2_key  = avx2_load_key(key);
  auto const avx2_null = avx2_load_key(null_key);
#endif
  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);
  for (;;) {
    T* keys = data + cache_line * line_values;
    T* vals = keys + line_pairs;
#ifdef __AVX2__
    auto const avx2_keys = avx2_load_keys(keys);
    auto       avx2_mask = avx2_cmp_keys<T>(avx2_key, avx2_keys);
    avx2_mask |= avx2_cmp_keys<T>(avx2_null, avx2_keys);
    if (avx2_mask) {
      auto const i = avx2_get_loc<T>(avx2_mask);
      return on_slot(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < line_pairs; ++i) {
      if (keys[i] == key) return on_slot(keys[i], vals[i]);
      if (keys[i] == null_key) return on_slot(keys[i], vals[i]);
    }
#endif
    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}
}

template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
class hash_map
{
  static constexpr u8 line_bytes = 64;
  static constexpr u8 pair_bytes = 2 * sizeof(T);

  T* m_data = nullptr;
  T  m_mask = 0;

public:
  hash_map() = default;
  explicit hash_map(
    arithmetic_concept auto size)
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

  /// inserts a key value pair or assigns to an existing key
  void insert_or_assign(
    T key,
    T val) noexcept
  {
    using namespace hash_map_internal;
    find_slot(m_data, m_mask, key, [&](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    });
  }

  /// write the value of an existing key
  void insert(
    T key,
    T val) noexcept
  {
    using namespace hash_map_internal;
    find_null(m_data, m_mask, key, [&](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    });
  }

  /// write the value of an existing key
  void try_insert(
    T key,
    T val) noexcept
  {
    using namespace hash_map_internal;
    auto const on_key  = [](auto /* key_ */, auto /* val_ */) {};
    auto const on_null = [=](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    };
    find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  /// write the value of an existing key
  void try_insert(
    T      key,
    auto&& val_producer) noexcept
  {
    using namespace hash_map_internal;
    auto const on_key  = [](auto /* key_ */, auto /* val_ */) {};
    auto const on_null = [&](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val_producer();
    };
    find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  /// write the value of an existing key
  void assign(
    T key,
    T val) noexcept
  {
    using namespace hash_map_internal;
    find_key(m_data, m_mask, key, [=](auto& key_slot, auto& val_slot) {
      key_slot = key;
      val_slot = val;
    });
  }

  /// finds the key and swaps the value
  auto swap(
    T key,
    T val) noexcept -> T
  {
    using namespace hash_map_internal;
    return find_key(m_data, m_mask, key, [=](auto& key_slot, auto& val_slot) {
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
    using namespace hash_map_internal;
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
    return find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  [[nodiscard]] auto get(
    T key) const noexcept -> T
  {
    using namespace hash_map_internal;
    return find_key(m_data, m_mask, key, [](auto /* key_ */, auto val_) { return val_; });
  }

  [[nodiscard]] auto get_default(
    T key,
    T val) const noexcept -> T
  {
    using namespace hash_map_internal;
    auto const on_key  = [](auto /* key_ */, auto val_) -> T { return val_; };
    auto const on_null = [=](auto /* key_ */, auto /* val_ */) -> T { return val; };
    return find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  [[nodiscard]] auto get_optional(
    T key) const noexcept -> std::optional<T>
  {
    using namespace hash_map_internal;
    auto const on_key  = [](auto /* key_ */, auto val_) -> std::optional<T> { return val_; };
    auto const on_null = [](auto /* key_ */, auto /* val_ */) -> std::optional<T> { return std::nullopt; };
    return find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  [[nodiscard]] auto get_or_insert(
    T key,
    T val) noexcept -> T
  {
    using namespace hash_map_internal;
    auto const on_key  = [](auto /* key_ */, auto val_) -> T { return val_; };
    auto const on_null = [=](auto& key_slot, auto& val_slot) -> T {
      key_slot = key;
      val_slot = val;
      return val;
    };
    return find_key_or_null(m_data, m_mask, key, on_key, on_null);
  }

  auto contains(
    T key) const noexcept -> bool
  {
    using namespace hash_map_internal;
    auto const on_key  = [](auto /* key_ */, auto /* val_ */) -> bool { return true; };
    auto const on_null = [](auto /* key_ */, auto /* val_ */) -> bool { return false; };
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
    static constexpr u8 line_pairs  = line_values / 2;

#ifdef __AVX2__
    using namespace hash_map_internal;
    auto const avx2_null = avx2_load_key(null_key);
#endif

    T c = 0;

    auto const end = static_cast<u64>(m_mask) + 1;
    for (u64 cache_line = 0; cache_line < end; ++cache_line) {
      T* keys = m_data + cache_line * line_values;
#ifdef __AVX2__
      auto const avx2_mask = avx2_cmp_keys<T>(avx2_null, avx2_load_keys(keys));
      ASSERT_GE(line_pairs, std::popcount(static_cast<u32>(avx2_mask)) / sizeof(T));
      c += line_pairs - (std::popcount(static_cast<u32>(avx2_mask)) / sizeof(T));
#else
      for (u8 i = 0; i < line_pairs; ++i) {
        if (keys[i] != null_key) ++c;
      }
#endif
    }

    return c;
  }
};

} // namespace kaspan
