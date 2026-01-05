#pragma once

#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/file_buffer.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/memory/page.hpp>
#include <kaspan/util/arithmetic.hpp>

namespace kaspan {

constexpr auto
representing_bytes(u64 max_val) -> u8
{
  constexpr auto one = static_cast<u64>(1);
  for (u8 bytes = 1; bytes < 8; ++bytes) {
    if (max_val < one << (bytes * 8)) {
      return bytes;
    }
  }
  return 8;
}

class buffer
{
public:
  buffer() noexcept = default;
  ~buffer()
  {
    if (data_ != nullptr) {
      DEBUG_ASSERT(is_line_aligned(data_));
      line_free(data_);
      data_ = nullptr;
    }
  }

  explicit buffer(u64 size) noexcept(false)
  {
    if (size != 0U) {
      data_ = line_alloc(size);
      DEBUG_ASSERT(is_line_aligned(data_));
      KASPAN_VALGRIND_MAKE_MEM_NOACCESS(data_, size);
    }
  }

  buffer(buffer const&) = delete;
  buffer(buffer&& rhs) noexcept
    : data_(rhs.data_)
  {

    rhs.data_ = nullptr;
  }

  auto operator=(buffer const&) -> buffer& = delete;
  auto operator=(buffer&& rhs) noexcept -> buffer&
  {
    if (this != &rhs) {
      if (data_ != nullptr) {
        DEBUG_ASSERT(is_line_aligned(data_));
        KASPAN_VALGRIND_FREELIKE_BLOCK(data_, 0);
        line_free(data_);
      }
      data_     = rhs.data_;
      rhs.data_ = nullptr;
    }
    return *this;
  }

  [[nodiscard]] auto data() const noexcept -> void*
  {
    return data_;
  }

protected:
  void* data_ = nullptr;
};

/**
 * @brief Allocate a buffer large enough to hold multiple contiguous arrays of the same element type.
 *
 * Computes total bytes as sum_i (sizes_i * sizeof(T)) and allocates a single buffer of that size.
 *
 * Template notes:
 * - The unused Disambiguator template parameter intentionally participates in template parameter lists
 *   to avoid ambiguous resolution against the heterogeneous variant below when only one size is passed.
 *
 * @tparam T Element type for all blocks (defaults to std::byte-like alias 'byte').
 * @tparam Disambiguator Always void; exists solely to steer overload resolution.
 * @param sizes The element counts per block.
 * @return buffer big enough for the concatenation of all blocks as bytes.
 * @throws std::bad_alloc On allocation failure.
 */
template<typename T = byte, std::same_as<void> Disambiguator = void>
auto
make_buffer(std::convertible_to<u64> auto... sizes) noexcept(false) -> buffer
  requires(sizeof...(sizes) > 0)
{
  static_assert(sizeof(T) > 0);
  static_assert(std::same_as<void, Disambiguator>);
  return buffer{ (line_align_up(sizes * sizeof(T)) + ...) };
}

/**
 * @brief Allocate a buffer for multiple contiguous arrays of possibly different element types.
 *
 * Computes total bytes as sum_i (sizes_i * sizeof(Ts_i)) and allocates a single buffer of that size.
 *
 * Constraints:
 * - Number of types equals number of sizes, and at least two blocks are specified.
 *
 * Rationale for Disambiguator:
 * - The unused Disambiguator template parameter exists to differentiate this overload from the homogeneously-typed
 *   overload above in otherwise ambiguous single-size/single-type scenarios.
 *
 * @tparam Ts Element types per block (one type per size).
 * @tparam Disambiguator Always void; exists solely to steer overload resolution.
 * @param sizes The element counts per block, in the same order as Ts...
 * @return buffer big enough for the concatenation of all blocks as bytes.
 * @throws std::bad_alloc On allocation failure.
 */
template<typename... Ts, std::same_as<void> Disambiguator = void>
auto
make_buffer(std::convertible_to<u64> auto... sizes) noexcept(false) -> buffer
  requires(sizeof...(Ts) == sizeof...(sizes) and sizeof...(sizes) > 1)
{
  static_assert(((sizeof(Ts) > 0) and ...));
  static_assert(std::same_as<void, Disambiguator>);
  return buffer{ (line_align_up(sizes * sizeof(Ts)) + ...) };
}

} // namespace kaspan
