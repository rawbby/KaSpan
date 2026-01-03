#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <unistd.h>

namespace kaspan {

namespace detail {
constexpr auto pagesize_default     = static_cast<u64>(4096);
inline bool    pagesize_initialized = false;
inline auto    pagesize             = pagesize_default;
}

inline auto
pagesize() -> u64
{
  if (detail::pagesize_initialized) [[likely]] { return detail::pagesize; }

  if (auto const sys_pagesize = sysconf(_SC_PAGESIZE); sys_pagesize > 0) [[likely]] { detail::pagesize = static_cast<u64>(sys_pagesize); }

  DEBUG_ASSERT_EQ(std::popcount(detail::pagesize), 1, "the page size is assumed to be a power of two");
  detail::pagesize_initialized = true;
  return detail::pagesize;
}

inline auto
page_align_down(u64 size)
{
  auto const mask = pagesize() - 1;
  return size & ~mask;
}

inline auto
page_align_up(u64 size)
{
  auto const mask = pagesize() - 1;
  return (size + mask) & ~mask;
}

inline auto
is_page_aligned(u64 size)
{
  auto const mask = pagesize() - 1;
  return (size & mask) == 0;
}

// clang-format off
static_assert(sizeof(void*) == sizeof(u64));
inline auto page_align_down(void const* data){return std::bit_cast<void const*>(page_align_down(std::bit_cast<u64>(data)));}
inline auto page_align_up(void const* data){return std::bit_cast<void const*>(page_align_up(std::bit_cast<u64>(data)));}
inline auto is_page_aligned(void const* data){return is_page_aligned(std::bit_cast<u64>(data));}
inline auto page_align_down(void* data){return std::bit_cast<void*>(page_align_down(std::bit_cast<u64>(data)));}
inline auto page_align_up(void* data){return std::bit_cast<void*>(page_align_up(std::bit_cast<u64>(data)));}
// clang-format on

[[nodiscard]] inline auto
page_alloc(u64 size) noexcept(false) -> void*
{
  DEBUG_ASSERT_GE(size, 0);
  auto const page = pagesize();
  auto const mask = page - 1;
  void*      data = std::aligned_alloc(page, (size + mask) & ~mask);
  if (data == nullptr) [[unlikely]] { throw std::bad_alloc{}; }
  return data;
}

inline void
page_free(void* data)
{
  DEBUG_ASSERT_NE(data, nullptr);
  DEBUG_ASSERT(is_page_aligned(data));
  std::free(data);
}

} // namespace kaspan
