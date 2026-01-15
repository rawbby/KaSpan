#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/pp.hpp>

#include <bit>
#include <cstdlib>
#include <limits>
#include <new>
#include <unistd.h>

namespace kaspan {

namespace detail {
constexpr auto linesize_default     = integral_cast<u32>(64);
inline bool    linesize_initialized = false;
inline auto    linesize_value       = linesize_default;
}

template<unsigned_concept Size = u32>
auto
linesize() -> Size
{
  if (!detail::linesize_initialized) [[unlikely]] {
    if (auto const sys_linesize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE); sys_linesize > 0) [[likely]] {
      DEBUG_ASSERT_IN_RANGE_INCLUSIVE(sys_linesize, std::numeric_limits<u32>::min(), std::numeric_limits<u32>::max());
      detail::linesize_value = integral_cast<u32>(sys_linesize);
    }
    DEBUG_ASSERT_EQ(std::popcount(detail::linesize_value), 1, "the cacheline size is assumed to be a power of two");
    detail::linesize_initialized = true;
  }

  DEBUG_ASSERT_LE(detail::linesize_value, std::numeric_limits<Size>::max());
  return integral_cast<Size>(detail::linesize_value);
}

template<unsigned_concept Size>
auto
line_align_down(
  Size size) -> Size
{
  auto const mask = linesize<Size>() - 1;
  return size & ~mask;
}

template<unsigned_concept Size>
auto
line_align_up(
  Size size) -> Size
{
  auto const mask = linesize<Size>() - 1;
  return (size + mask) & ~mask;
}

template<unsigned_concept Size>
auto
is_line_aligned(
  Size size) -> bool
{
  auto const mask = linesize<Size>() - 1;
  return (size & mask) == 0;
}

// clang-format off
static_assert(sizeof(void*)==sizeof(u64));
inline auto line_align_down(void const* data){return std::bit_cast<void const*>(line_align_down(std::bit_cast<u64>(data)));}
inline auto line_align_up(void const* data){return std::bit_cast<void const*>(line_align_up(std::bit_cast<u64>(data)));}
inline auto is_line_aligned(void const* data){return is_line_aligned(std::bit_cast<u64>(data));}
inline auto line_align_down(void* data){return std::bit_cast<void*>(line_align_down(std::bit_cast<u64>(data)));}
inline auto line_align_up(void* data){return std::bit_cast<void*>(line_align_up(std::bit_cast<u64>(data)));}
// clang-format on

template<arithmetic_concept Size>
[[nodiscard]] auto
line_alloc(Size size) noexcept(false) -> void*
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = integral_cast<u64>(size);
  if (size64 == 0) {
    return nullptr;
  }
  auto const line = linesize();
  auto const mask = line - 1;
  void*      data = std::aligned_alloc(line, (size64 + mask) & ~mask);
  if (data == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }
  return data;
}

inline void
line_free(
  void* data)
{
  if (data != nullptr) {
    DEBUG_ASSERT(is_line_aligned(data));
    std::free(data);
  }
}

} // namespace kaspan
