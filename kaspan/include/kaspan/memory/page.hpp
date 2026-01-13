#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <bit>
#include <unistd.h>

namespace kaspan {

namespace detail {
constexpr auto pagesize_default     = integral_cast<u32>(4096);
inline bool    pagesize_initialized = false;
inline auto    pagesize_value       = pagesize_default;
}

template<unsigned_concept Size = u32>
auto
pagesize() -> Size
{
  if (!detail::pagesize_initialized) [[unlikely]] {
    if (auto const sys_pagesize = sysconf(_SC_PAGESIZE); sys_pagesize > 0) [[likely]] {
      DEBUG_ASSERT_IN_RANGE_INCLUSIVE(sys_pagesize, std::numeric_limits<u32>::min(), std::numeric_limits<u32>::max());
      detail::pagesize_value = integral_cast<u32>(sys_pagesize);
    }
    DEBUG_ASSERT_EQ(std::popcount(detail::pagesize_value), 1, "the page size is assumed to be a power of two");
    detail::pagesize_initialized = true;
  }

  DEBUG_ASSERT_LE(detail::pagesize_value, std::numeric_limits<Size>::max());
  return integral_cast<Size>(detail::pagesize_value);
}

template<unsigned_concept Size>
auto
page_align_down(Size size) -> Size
{
  auto const mask = pagesize<Size>() - 1;
  return size & ~mask;
}

template<unsigned_concept Size>
auto
page_align_up(Size size) -> Size
{
  auto const mask = pagesize<Size>() - 1;
  return (size + mask) & ~mask;
}

template<unsigned_concept Size>
auto
is_page_aligned(Size size) -> bool
{
  auto const mask = pagesize<Size>() - 1;
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

template<arithmetic_concept Size>
[[nodiscard]] auto
page_alloc(Size size) noexcept(false) -> void*
{
  DEBUG_ASSERT_GE(size, 0);
  DEBUG_ASSERT_LE(size, std::numeric_limits<u64>::max());
  auto const size64 = integral_cast<u64>(size);
  if (size64 == 0) {
    return nullptr;
  }
  auto const page = pagesize();
  auto const mask = page - 1;
  void*      data = std::aligned_alloc(page, (size64 + mask) & ~mask);
  if (data == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }
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
