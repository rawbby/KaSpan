#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/hash_util.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>
#include <optional>

namespace kaspan {

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
    return find_key(key, [=](auto& val_) {
      auto const val_ = val_;
      val_            = val;
      return val_;
    });
  }

  /// finds the key if inserted and swaps the value else insert
  auto swap_or_insert(
    T key,
    T val) noexcept -> std::optional<T>
  {
    auto const on_key = [=](auto& val_) -> std::optional<T> {
      auto const val_ = val_;
      val_            = val;
      return val_;
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
