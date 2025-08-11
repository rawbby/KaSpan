#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <ostream>
#include <unistd.h>

#include "ErrorCode.hpp"

inline Result<std::size_t>
page_count()
{
  const auto physical_pages = sysconf(_SC_PHYS_PAGES);
  ASSERT_TRY(physical_pages > 0, IO_ERROR);
  return static_cast<std::size_t>(physical_pages);
}

inline Result<std::size_t>
page_size()
{
  const auto page_size = sysconf(_SC_PAGESIZE);
  ASSERT_TRY(page_size > 0, IO_ERROR);
  return static_cast<std::size_t>(page_size);
}

inline Result<std::size_t>
page_aligned_size(std::size_t size)
{
  RESULT_TRY(const auto page_size, page_size());
  return (size + page_size - 1) / page_size * page_size;
}

inline Result<std::byte*>
page_aligned_alloc(std::size_t size)
{
  if (size == 0)
    return nullptr;
  RESULT_TRY(const auto page_aligned_size, page_aligned_size(size));
  RESULT_TRY(const auto page_size, page_size());
  const auto data = std::aligned_alloc(page_size, page_aligned_size);
  ASSERT_TRY(data, ALLOCATION_ERROR);
  return static_cast<std::byte*>(data);
}

template<typename T>
  requires std::is_unsigned_v<T>
constexpr T
from_little_endian(T val)
{
  if constexpr (sizeof(T) > 1 && std::endian::native == std::endian::big) {
    // ReSharper disable once CppDFAUnreachableCode
    auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(val);
    std::ranges::reverse(bytes);
    val = std::bit_cast<T>(bytes);
  }
  return val;
}

template<typename T>
  requires std::is_unsigned_v<T>
constexpr T
to_little_endian(T val)
{
  return from_little_endian(val);
}

template<typename Dst>
  requires(std::is_unsigned_v<Dst>)
Dst
read_from_little_endian(const std::byte* src, std::size_t len)
{
  Dst dst = 0;
  memcpy(&dst, src, len);
  return from_little_endian(dst);
}

template<typename Src>
  requires(std::is_unsigned_v<Src>)
void
write_little_endian(std::byte* dst, Src src, std::size_t len)
{
  src = to_little_endian(src);
  memcpy(dst, &src, len);
}

constexpr std::uint8_t
needed_bytes(std::uint64_t max_val)
{
  constexpr auto one = static_cast<std::uint64_t>(1);
  for (std::uint8_t bytes = 1; bytes < 8; ++bytes)
    if (max_val < one << (bytes * 8))
      return bytes;
  return 8;
}

template<typename T>
  requires std::is_unsigned_v<T>
VoidResult
write_little_endian(std::ostream& out, T val, std::uint8_t bytes)
{
  val = to_little_endian(val);
  out.write(reinterpret_cast<const char*>(&val), bytes);
  ASSERT_TRY(out, IO_ERROR);
  return VoidResult::success();
}

template<typename T>
  requires std::is_unsigned_v<T>
Result<T>
read_little_endian(std::istream& in, std::uint8_t bytes)
{
  T val = 0;
  in.read(reinterpret_cast<char*>(&val), bytes);
  ASSERT_TRY(in, IO_ERROR);
  return from_little_endian(val);
}
