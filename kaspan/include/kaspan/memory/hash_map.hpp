#pragma once

#include <kaspan/memory/hash_container_avx.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>
#include <optional>

namespace kaspan {

template<integral_c T>
auto
hash_map_find_null(
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
    if (auto const avx_mask = avx2_cmpeq_keys<T>(avx_null, avx2_load_keys(keys))) {
      auto const i = avx2_mask_first<T>(avx_mask);
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
hash_map_find_key(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key)
{
#if defined(__AVX2__)
  auto const avx_key = avx2_set1_key(key);
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    if (auto const avx_mask = avx2_cmpeq_keys<T>(avx_key, avx2_load_keys(keys))) {
      return on_key(vals[avx2_mask_first<T>(avx_mask)]);
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
hash_map_find_key_or_null(
  T*     data,
  T      mask,
  T      key,
  auto&& on_key,
  auto&& on_null)
{
#if defined(__AVX2__)
  auto const avx_key  = avx2_set1_key(key);
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    auto const avx_keys = avx2_load_keys(keys);
    if (auto const mask_key = avx2_cmpeq_keys<T>(avx_key, avx_keys)) {
      auto const i = avx2_mask_first<T>(mask_key);
      return on_key(vals[i]);
    }
    if (auto const mask_null = avx2_cmpeq_keys<T>(avx_null, avx_keys)) {
      auto const i = avx2_mask_first<T>(mask_null);
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
hash_map_find_slot(
  T*     data,
  T      mask,
  T      key,
  auto&& on_slot)
{
#if defined(__AVX2__)
  auto const avx_key  = avx2_set1_key(key);
  auto const avx_null = avx2_set1_key(static_cast<T>(-1));
#endif

  auto cache_line = hash(key) & mask;
  IF(KASPAN_DEBUG, auto const first_cache_line = cache_line);

  for (;;) {
    T* keys = data + cache_line * 64 / sizeof(T);
    T* vals = keys + 32 / sizeof(T);

#if defined(__AVX2__)
    auto const avx_keys = avx2_load_keys(keys);
    auto const avx_mask = avx_mask_lor(avx2_cmpeq_keys<T>(avx_key, avx_keys), avx2_cmpeq_keys<T>(avx_null, avx_keys));
    if (avx_mask) {
      auto const i = avx2_mask_first<T>(avx_mask);
      return on_slot(keys[i], vals[i]);
    }
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      if (keys[i] == key || keys[i] == static_cast<T>(-1)) return on_slot(keys[i], vals[i]);
    }
#endif

    cache_line = cache_line + 1 & mask;
    DEBUG_ASSERT_NE(cache_line, first_cache_line);
  }
}

template<integral_c T>
[[nodiscard]] auto
hash_map_count_null(
  T* data,
  T  mask) noexcept -> T
{
#if defined(__AVX2__)
  auto const avx_null = avx2_set1_key<T>(static_cast<T>(-1));
#endif

  T count = 0;

  auto const end = static_cast<u64>(mask) + 1;
  for (u64 it = 0; it < end; ++it) {
    T* keys = data + it * 64 / sizeof(T);

#if defined(__AVX2__)
    auto const avx_keys = avx2_load_keys<T>(keys);
    count += avx2_mask_count<T>(avx2_cmpeq_keys<T>(avx_null, avx_keys));
#else
    for (u32 i = 0; i < 32 / sizeof(T); ++i) {
      count += keys[i] == static_cast<T>(-1);
    }
#endif
  }

  return count;
}

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
      auto const cache_lines = std::bit_ceil(ceildiv<line_bytes>(integral_cast<u64>(size) * (3 * pair_bytes / 2)));
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
    find_slot(key, [&](auto& key_, auto& val_) {
      key_ = key;
      val_ = val;
    });
  }

  /// write the value of an existing key
  void insert(
    T key,
    T val) noexcept
  {
    find_null(key, [&](auto& key_, auto& val_) {
      key_ = key;
      val_ = val;
    });
  }

  /// write the value of an existing key
  void try_insert(
    T key,
    T val) noexcept
  {
    auto const on_key  = [](auto /* val_ */) {};
    auto const on_null = [=](auto& key_, auto& val_) {
      key_ = key;
      val_ = val;
    };
    find_key_or_null(key, on_key, on_null);
  }

  /// write the value of an existing key
  void try_insert(
    T      key,
    auto&& val_producer) noexcept
  {
    auto const on_key  = [](auto /* val_ */) {};
    auto const on_null = [&](auto& key_, auto& val_) {
      key_ = key;
      val_ = val_producer();
    };
    find_key_or_null(key, on_key, on_null);
  }

  /// write the value of an existing key
  void assign(
    T key,
    T val) noexcept
  {
    find_key(key, [=](auto& val_) { val_ = val; });
  }

  /// finds the key and swaps the value
  auto swap(
    T key,
    T val) noexcept -> T
  {
    return find_key(key, [=](auto& val_slot) {
      auto const old_val = val_slot;
      val_slot           = val;
      return old_val;
    });
  }

  /// finds the key if inserted and swaps the value else insert
  auto swap_or_insert(
    T key,
    T val) noexcept -> std::optional<T>
  {
    auto const on_key = [=](auto& val_slot) -> std::optional<T> {
      auto const old_val = val_slot;
      val_slot           = val;
      return old_val;
    };
    auto const on_null = [=](auto& key_, auto& val_) -> std::optional<T> {
      key_ = key;
      val_ = val;
      return std::nullopt;
    };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get(
    T key) noexcept -> T
  {
    return find_key(key, [](auto val_) { return val_; });
  }

  [[nodiscard]] auto get_default(
    T key,
    T val) noexcept -> T
  {
    auto const on_key  = [](auto val_) -> T { return val_; };
    auto const on_null = [=](auto /* key_ */, auto /* val_ */) -> T { return val; };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get_optional(
    T key) noexcept -> std::optional<T>
  {
    auto const on_key  = [](auto val_) -> std::optional<T> { return val_; };
    auto const on_null = [](auto /* key_ */, auto /* val_ */) -> std::optional<T> { return std::nullopt; };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get_or_insert(
    T      key,
    auto&& val_producer) noexcept -> T
  {
    auto const on_key  = [](auto val_) -> T { return val_; };
    auto const on_null = [=](auto& key_, auto& val_) -> T {
      key_        = key;
      return val_ = val_producer();
    };
    return find_key_or_null(key, on_key, on_null);
  }

  [[nodiscard]] auto get_or_insert(
    T key,
    T val) noexcept -> T
  {
    auto const on_key  = [](auto val_) -> T { return val_; };
    auto const on_null = [=](auto& key_, auto& val_) -> T {
      key_        = key;
      return val_ = val;
    };
    return find_key_or_null(key, on_key, on_null);
  }

  auto contains(
    T key) noexcept -> bool
  {
    auto const on_key  = [](auto...) -> bool { return true; };
    auto const on_null = [](auto...) -> bool { return false; };
    return find_key_or_null(key, on_key, on_null);
  }

  void clear() noexcept
  {
    if (m_data) std::memset(m_data, 0xff, (integral_cast<u64>(m_mask) + 1) * line_bytes);
  }

  [[nodiscard]] auto count() noexcept -> T
  {
    auto const slot_sount = (static_cast<u64>(m_mask) + 1) * (32 / sizeof(T));
    return slot_sount - hash_map_count_null(m_data, m_mask);
  }

  auto find_null(
    T      key,
    auto&& on_null) noexcept
  {
    return hash_map_find_null(m_data, m_mask, key, std::forward<decltype(on_null)>(on_null));
  }

  auto find_key(
    T      key,
    auto&& on_key) noexcept
  {
    return hash_map_find_key(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key));
  }

  auto find_key_or_null(
    T      key,
    auto&& on_key,
    auto&& on_null) noexcept
  {
    return hash_map_find_key_or_null(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key), std::forward<decltype(on_null)>(on_null));
  }

  auto find_slot(
    T      key,
    auto&& on_slot) noexcept
  {
    return hash_map_find_slot(m_data, m_mask, key, std::forward<decltype(on_slot)>(on_slot));
  }
};

} // namespace kaspan
