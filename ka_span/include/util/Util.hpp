#pragma once

#include <util/Result.hpp>

#include <algorithm>
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
  ASSERT_TRY(lstat(file, &st) == 0, FILESYSTEM_ERROR);
  return static_cast<u64>(st.st_size);
}

inline auto
file_size_fd(int fd) -> Result<u64>
{
  struct stat st{};
  ASSERT_TRY(fstat(fd, &st) == 0, FILESYSTEM_ERROR);
  ASSERT_TRY(S_ISREG(st.st_mode), FILESYSTEM_ERROR);
  return static_cast<u64>(st.st_size);
}

inline auto
page_count() -> Result<size_t>
{
  auto const physical_pages = sysconf(_SC_PHYS_PAGES);
  ASSERT_TRY(physical_pages > 0, IO_ERROR);
  return static_cast<size_t>(physical_pages);
}

inline auto
page_size() -> Result<size_t>
{
  auto const page_size = sysconf(_SC_PAGESIZE);
  ASSERT_TRY(page_size > 0, IO_ERROR);
  return static_cast<size_t>(page_size);
}

inline auto
page_aligned_size(size_t size) -> Result<size_t>
{
  RESULT_TRY(auto const page_size, page_size());
  return (size + page_size - 1) / page_size * page_size;
}

inline auto
page_aligned_alloc(size_t bytes) -> Result<void*>
{
  if (bytes == 0)
    return nullptr;
  RESULT_TRY(auto const page_aligned_size, page_aligned_size(bytes));
  RESULT_TRY(auto const page_size, page_size());
  auto const data = std::aligned_alloc(page_size, page_aligned_size);
  ASSERT_TRY(data, ALLOCATION_ERROR);
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
