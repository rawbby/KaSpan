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
    return slot_sount - hash_map_util::count256_null(m_data, m_mask);
  }

  auto find_null(
    T      key,
    auto&& on_null) noexcept
  {
    return hash_map_util::find256_null(m_data, m_mask, key, std::forward<decltype(on_null)>(on_null));
  }

  auto find_key(
    T      key,
    auto&& on_key) noexcept
  {
    return hash_map_util::find256_key(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key));
  }

  auto find_key_or_null(
    T      key,
    auto&& on_key,
    auto&& on_null) noexcept
  {
    return hash_map_util::find256_key_or_null(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key), std::forward<decltype(on_null)>(on_null));
  }

  auto find_slot(
    T      key,
    auto&& on_slot) noexcept
  {
    return hash_map_util::find256_slot(m_data, m_mask, key, std::forward<decltype(on_slot)>(on_slot));
  }
};

} // namespace kaspan
