#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <unistd.h>

namespace kaspan {

namespace detail {
constexpr auto linesize_default     = static_cast<u64>(64);
inline bool    linesize_initialized = false;
inline auto    linesize             = linesize_default;
}

inline auto
linesize() -> u64
{
  if (detail::linesize_initialized) [[likely]] {
    return detail::linesize;
  }

  if (auto const sys_linesize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE); sys_linesize > 0) [[likely]] {
    detail::linesize = static_cast<u64>(sys_linesize);
  }

  DEBUG_ASSERT_EQ(std::popcount(detail::linesize), 1, "the cacheline size is assumed to be a power of two");
  detail::linesize_initialized = true;
  return detail::linesize;
}

inline auto
line_align_down(u64 size)
{
  auto const mask = linesize() - 1;
  return size & ~mask;
}

inline auto
line_align_up(u64 size)
{
  auto const mask = linesize() - 1;
  return (size + mask) & ~mask;
}

inline auto
is_line_aligned(u64 size)
{
  auto const mask = linesize() - 1;
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

[[nodiscard]] inline auto
line_alloc(u64 size) noexcept(false) -> void*
{
  DEBUG_ASSERT_GE(size, 0);
  if (size == 0) {
    return nullptr;
  }
  auto const line = linesize();
  auto const mask = line - 1;
  void*      data = std::aligned_alloc(line, (size + mask) & ~mask);
  if (data == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }
  KASPAN_VALGRIND_MALLOCLIKE_BLOCK(data, size, 0, 0);
  return data;
}

inline void
line_free(void* data)
{
  DEBUG_ASSERT_NE(data, nullptr);
  DEBUG_ASSERT(is_line_aligned(data));
  KASPAN_VALGRIND_FREELIKE_BLOCK(data, 0);
  std::free(data);
}

} // namespace kaspan
