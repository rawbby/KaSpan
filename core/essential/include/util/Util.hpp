#pragma once

#include <util/Result.hpp>

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <istream>
#include <ranges>
#include <sys/stat.h>
#include <unistd.h>

inline auto
file_size(char const* file) -> Result<u64>
{
  struct stat st{};
  RESULT_ASSERT(lstat(file, &st) == 0, FILESYSTEM_ERROR);
  return static_cast<u64>(st.st_size);
}

inline auto
file_size_fd(int fd) -> Result<u64>
{
  struct stat st{};
  RESULT_ASSERT(fstat(fd, &st) == 0, FILESYSTEM_ERROR);
  RESULT_ASSERT(S_ISREG(st.st_mode), FILESYSTEM_ERROR);
  return static_cast<u64>(st.st_size);
}

inline auto
page_count() -> Result<size_t>
{
  auto const physical_pages = sysconf(_SC_PHYS_PAGES);
  RESULT_ASSERT(physical_pages > 0, IO_ERROR);
  return static_cast<size_t>(physical_pages);
}

inline auto
page_size() -> u64
{
  static auto const ps = []() -> u64 {
    auto const sys_ps = sysconf(_SC_PAGESIZE);
    if (sys_ps > 0) {
      return sys_ps;
    }
    return 4096;
  }();
  return ps;
}

inline auto
page_aligned_size(size_t size) -> Result<size_t>
{
  auto const ps = page_size();
  return (size + ps - 1) / ps * ps;
}

inline auto
page_ceil(u64 size) -> u64
{
  auto const ps = page_size();
  return (size + ps - 1) / ps * ps;
}

inline auto
page_align_ceil(void const* data) -> void const*
{
  return std::bit_cast<void*>(page_ceil(std::bit_cast<u64>(data)));
}

inline auto
page_align_ceil(void* data) -> void*
{
  return std::bit_cast<void*>(page_ceil(std::bit_cast<u64>(data)));
}

inline auto
page_floor(u64 size) -> u64
{
  auto const ps = page_size();
  return size / ps * ps;
}

inline auto
page_align_floor(void const* data) -> void const*
{
  return std::bit_cast<void*>(page_floor(std::bit_cast<u64>(data)));
}

inline auto
page_align_floor(void* data) -> void*
{
  return std::bit_cast<void*>(page_floor(std::bit_cast<u64>(data)));
}

inline auto
page_aligned(void const* data) -> bool
{
  return data == page_align_floor(data);
}

inline auto
page_aligned_alloc(size_t bytes) -> Result<void*>
{
  if (bytes == 0)
    return nullptr;
  RESULT_TRY(auto const page_aligned_size, page_aligned_size(bytes));
  auto const ps   = page_size();
  auto const data = std::aligned_alloc(ps, page_aligned_size);
  RESULT_ASSERT(data, ALLOCATION_ERROR);
  return data;
}

constexpr auto
needed_bytes(u64 max_val) -> u8
{
  constexpr auto one = static_cast<u64>(1);
  for (u8 bytes = 1; bytes < 8; ++bytes)
    if (max_val < one << (bytes * 8))
      return bytes;
  return 8;
}
