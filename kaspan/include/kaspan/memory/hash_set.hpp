#pragma once

#include <kaspan/memory/hash_container_avx.hpp>
#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>

namespace kaspan {

template<integral_c T>
auto
hash_set_find_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_null)
{
#if defined(__AVX512F__)
  auto const avx_null = avx512_set1_key(static_cast<T>(-1));
#elif defined(__AVX2__)
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);

#if defined(__AVX512F__)
    if (auto const avx_mask = avx512_cmpeq_keys<T>(avx_null, avx512_load_keys(keys))) {
      return on_null(keys[avx512_mask_first<T>(avx_mask)]);
    }
#elif defined(__AVX2__)
    auto const avx_mask_lo = avx2_cmpeq_keys<T>(avx_null, avx2_load_keys(keys));
    auto const avx_mask_hi = avx2_cmpeq_keys<T>(avx_null, avx2_load_keys(keys + 32 / sizeof(T)));
    if (auto const avx_mask = avx2_mask_combine(avx_mask_lo, avx_mask_hi)) {
      return on_null(keys[avx2_mask_first<T>(avx_mask)]);
    }
#else
    for (u32 i = 0; i < 64 / sizeof(T); ++i) {
      if (keys[i] == static_cast<T>(-1)) return on_null(keys[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
hash_set_find_key(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key)
{
#if defined(__AVX512F__)
  auto const avx_key = avx512_set1_key(key);
#elif defined(__AVX2__)
  auto const avx_key = avx2_set1_key(key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);

#if defined(__AVX512F__)
    if (auto const avx_mask = avx512_cmpeq_keys<T>(avx_key, avx512_load_keys(keys))) {
      return on_key(keys[avx512_mask_first<T>(avx_mask)]);
    }
#elif defined(__AVX2__)
    auto const avx_mask_lo = avx2_cmpeq_keys<T>(avx_key, avx2_load_keys(keys));
    auto const avx_mask_hi = avx2_cmpeq_keys<T>(avx_key, avx2_load_keys(keys + 32 / sizeof(T)));
    if (auto const avx_mask = avx2_mask_combine(avx_mask_lo, avx_mask_hi)) {
      return on_key(keys[avx2_mask_first<T>(avx_mask)]);
    }
#else
    for (u32 i = 0; i < 64 / sizeof(T); ++i) {
      if (keys[i] == key) return on_key(keys[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
hash_set_find_key_or_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key,
  auto&& on_null)
{
#if defined(__AVX512F__)
  auto const avx_key  = avx512_set1_key(key);
  auto const avx_null = avx512_set1_key(static_cast<T>(-1));
#elif defined(__AVX2__)
  auto const avx_key  = avx2_set1_key(key);
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);

#if defined(__AVX512F__)
    auto const avx_keys = avx512_load_keys(keys);
    if (auto const avx_mask = avx512_cmpeq_keys<T>(avx_key, avx_keys)) {
      return on_key(keys[avx512_mask_first<T>(avx_mask)]);
    }
    if (auto const avx_mask = avx512_cmpeq_keys<T>(avx_null, avx_keys)) {
      return on_null(keys[avx512_mask_first<T>(avx_mask)]);
    }
#elif defined(__AVX2__)
    auto const avx_keys_lo = avx2_load_keys(keys);
    auto const avx_keys_hi = avx2_load_keys(keys + 32 / sizeof(T));
    {
      auto const avx_mask_lo = avx2_cmpeq_keys<T>(avx_key, avx_keys_lo);
      auto const avx_mask_hi = avx2_cmpeq_keys<T>(avx_key, avx_keys_hi);
      if (auto const avx_mask = avx2_mask_combine(avx_mask_lo, avx_mask_hi)) {
        return on_key(keys[avx2_mask_first<T>(avx_mask)]);
      }
    }
    {
      auto const avx_mask_lo = avx2_cmpeq_keys<T>(avx_null, avx_keys_lo);
      auto const avx_mask_hi = avx2_cmpeq_keys<T>(avx_null, avx_keys_hi);
      if (auto const avx_mask = avx2_mask_combine(avx_mask_lo, avx_mask_hi)) {
        return on_null(keys[avx2_mask_first<T>(avx_mask)]);
      }
    }
#else
    for (u32 i = 0; i < 64 / sizeof(T); ++i) {
      if (keys[i] == key) return on_key(keys[i]);
      if (keys[i] == static_cast<T>(-1)) return on_null(keys[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
auto
hash_set_find_slot(
  T*     data,
  T      mask,
  T      key,
  auto&& on_slot)
{
#if defined(__AVX512F__)
  auto const avx_key  = avx512_set1_key(key);
  auto const avx_null = avx512_set1_key(static_cast<T>(-1));
#elif defined(__AVX2__)
  auto const avx_key  = avx2_set1_key(key);
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);

#if defined(__AVX512F__)
    auto const avx_keys      = avx512_load_keys(keys);
    auto const avx_mask_key  = avx512_cmpeq_keys<T>(avx_key, avx_keys);
    auto const avx_mask_null = avx512_cmpeq_keys<T>(avx_null, avx_keys);
    if (auto const avx_mask = avx_mask_lor(avx_mask_key, avx_mask_null)) {
      return on_slot(keys[avx512_mask_first<T>(avx_mask)]);
    }
#elif defined(__AVX2__)
    auto const avx_keys_lo = avx2_load_keys(keys);
    auto const avx_keys_hi = avx2_load_keys(keys + 32 / sizeof(T));
    auto const avx_mask_lo = avx_mask_lor(avx2_cmpeq_keys<T>(avx_key, avx_keys_lo), avx2_cmpeq_keys<T>(avx_null, avx_keys_lo));
    auto const avx_mask_hi = avx_mask_lor(avx2_cmpeq_keys<T>(avx_key, avx_keys_hi), avx2_cmpeq_keys<T>(avx_null, avx_keys_hi));
    if (auto const avx_mask = avx2_mask_combine(avx_mask_lo, avx_mask_hi)) {
      return on_slot(keys[avx2_mask_first<T>(avx_mask)]);
    }
#else
    for (u32 i = 0; i < 64 / sizeof(T); ++i) {
      if (keys[i] == key || keys[i] == static_cast<T>(-1)) return on_slot(keys[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
[[nodiscard]] auto
hash_set_count_null(
  T* data,
  T  mask) noexcept -> T
{
#if defined(__AVX512F__)
  auto const avx_null = avx512_set1_key<T>(static_cast<T>(-1));
#elif defined(__AVX2__)
  auto const avx_null = avx2_set1_key<T>(static_cast<T>(-1));
#endif

  T count = 0;

  auto const end = static_cast<u64>(mask) + 1;
  for (u64 it = 0; it < end; ++it) {
    T* keys = data + it * 64 / sizeof(T);

#if defined(__AVX512F__)
    auto const avx_keys = avx512_load_keys(keys);
    count += avx512_mask_count(avx512_cmpeq_keys<T>(avx_null, avx_keys));
#elif defined(__AVX2__)
    auto const keys_lo = avx2_load_keys<T>(keys);
    count += avx2_mask_count<T>(avx2_cmpeq_keys<T>(avx_null, keys_lo));
    auto const keys_hi = avx2_load_keys<T>(keys + 32 / sizeof(T));
    count += avx2_mask_count<T>(avx2_cmpeq_keys<T>(avx_null, keys_hi));
#else
    for (u32 i = 0; i < 64 / sizeof(T); ++i) {
      count += keys[i] == static_cast<T>(-1);
    }
#endif
  }

  return count;
}

template<typename T>
  requires(u32_c<T> || u64_c<T>)
class hash_set
{

  T* m_data = nullptr;
  T  m_mask = 0;

public:
  hash_set() = default;
  explicit hash_set(
    integral_c auto size)
  {
    if (size) {
      auto const cache_lines = std::bit_ceil(ceildiv<64>(integral_cast<u64>(size) * (3 * sizeof(T) / 2)));
      auto const alloc_bytes = cache_lines * 64;

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
    find_slot(key, [&](auto& key_slot) { key_slot = key; });
  }

  /// returns true if the key was newly inserted
  auto try_insert(
    T key) noexcept -> bool
  {
    auto const on_key  = [](auto /* key_slot */) { return false; };
    auto const on_null = [=](auto& key_slot) {
      key_slot = key;
      return true;
    };
    return find_key_or_null(key, on_key, on_null);
  }

  auto contains(
    T key) const noexcept -> bool
  {
    auto const on_key  = [](auto /* key_slot */) -> bool { return true; };
    auto const on_null = [](auto /* key_slot */) -> bool { return false; };
    return find_key_or_null(key, on_key, on_null);
  }

  void clear() noexcept
  {
    if (m_data) std::memset(m_data, 0xff, (integral_cast<u64>(m_mask) + 1) * 64);
  }

  [[nodiscard]] auto count() const noexcept -> T
  {
    auto const slot_sount = (static_cast<u64>(m_mask) + 1) * (64 / sizeof(T));
    return slot_sount - hash_set_count_null(m_data, m_mask);
  }

  auto find_null(
    T      key,
    auto&& on_null) noexcept
  {
    return hash_set_find_null(m_data, m_mask, key, std::forward<decltype(on_null)>(on_null));
  }

  auto find_key(
    T      key,
    auto&& on_key) const noexcept
  {
    return hash_set_find_key(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key));
  }

  auto find_key_or_null(
    T      key,
    auto&& on_key,
    auto&& on_null) const noexcept
  {
    return hash_set_find_key_or_null(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key), std::forward<decltype(on_null)>(on_null));
  }

  auto find_slot(
    T      key,
    auto&& on_slot) noexcept
  {
    return hash_set_find_slot(m_data, m_mask, key, std::forward<decltype(on_slot)>(on_slot));
  }
};

} // namespace kaspan
