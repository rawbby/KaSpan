#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/hash_util.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/hash.hpp>
#include <kaspan/util/math.hpp>

#include <bit>
#include <cstring>

namespace kaspan {

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
    if (m_data) std::memset(m_data, 0xff, (integral_cast<u64>(m_mask) + 1) * line_bytes);
  }

  [[nodiscard]] auto count() const noexcept -> T
  {
    return static_cast<u64>(m_mask) - hash_map_util::count512_null(m_data, m_mask) + 1;
  }

  auto find_null(
    T      key,
    auto&& on_null) noexcept
  {
    return hash_map_util::find512_null(m_data, m_mask, key, std::forward<decltype(on_null)>(on_null));
  }

  auto find_key(
    T      key,
    auto&& on_key) const noexcept
  {
    return hash_map_util::find512_key(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key));
  }

  auto find_key_or_null(
    T      key,
    auto&& on_key,
    auto&& on_null) const noexcept
  {
    return hash_map_util::find512_key_or_null(m_data, m_mask, key, std::forward<decltype(on_key)>(on_key), std::forward<decltype(on_null)>(on_null));
  }

  auto find_slot(
    T      key,
    auto&& on_slot) noexcept
  {
    return hash_map_util::find512_slot(m_data, m_mask, key, std::forward<decltype(on_slot)>(on_slot));
  }
};

} // namespace kaspan
