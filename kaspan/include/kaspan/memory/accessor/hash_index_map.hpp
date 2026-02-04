#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/hash_map.hpp>
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

/// special hashmap that maps global indices to continuous local ones.
/// only inserted keys are allowed to be queried.
/// capacity must at least be half the number of elements to store (100% oppacity).
/// further insertions are undefined behaviour (infinite loop).
/// uses linear probing so the default capacity is twice the size requested (hard-coded).
template<typename T>
  requires(u32_concept<T> || u64_concept<T>)
class hash_index_map
{
  hash_map<T> m_map{};
  T           m_count = 0;

public:
  hash_index_map() = default;

  explicit hash_index_map(
    arithmetic_concept auto size)
    : m_map(size)
  {
  }

  ~hash_index_map() = default;

  hash_index_map(hash_index_map const&)         = delete;
  hash_index_map(hash_index_map&& rhs) noexcept = default;

  hash_index_map& operator=(hash_index_map const&)         = delete;
  hash_index_map& operator=(hash_index_map&& rhs) noexcept = default;

  void try_insert(
    T key) noexcept
  {
    return m_map.try_insert(key, [&] { return m_count++; });
  }

  [[nodiscard]] auto get(
    T key) const noexcept -> T
  {
    return m_map.get(key);
  }

  [[nodiscard]] auto count() const noexcept -> T
  {
    return m_count;
  }

  void clear()
  {
    m_map.clear();
  }
};

} // namespace kaspan
