#pragma once

#include <bit>         // bit_cast endian bit_width bit_ceil
#include <concepts>    // unsigned_integral
#include <cstddef>     // byte size_t
#include <cstdint>     // uint8_t uint64_t
#include <cstdio>      // FILE fread fwrite
#include <ranges>      // ranges::contiguous_range ranges::size ranges::data
#include <span>        // span as_writable_bytes
#include <type_traits> // remove_reference_t
#include <utility>     // swap

namespace csr::util {

namespace endian {
auto
swap(std::unsigned_integral auto& value)
{
  constexpr auto half_size = static_cast<std::uint8_t>(sizeof(value) >> 1);

  auto raw = std::as_writable_bytes(std::span{ &value, 1 }).data();
  for (std::uint8_t i = 0; i < half_size; ++i)
    std::swap(raw[i], raw[sizeof(value) - i - 1]);
}
}

void
read(FILE* file, std::unsigned_integral auto& value, std::uint8_t bytes = sizeof(value))
{
  using value_t = std::remove_reference_t<decltype(value)>;

  std::byte raw[sizeof(value_t)] = {};
  fread(raw, 1, bytes, file);

  value = std::bit_cast<value_t>(raw);
  if constexpr (std::endian::native == std::endian::big)
    endian::swap(value);
}

void
write(FILE* file, std::unsigned_integral auto value, std::uint8_t bytes = sizeof(value))
{
  if constexpr (std::endian::native == std::endian::big)
    endian::swap(value);

  auto raw = std::as_writable_bytes(std::span{ &value, 1 }).data();
  fwrite(raw, 1, bytes, file);
}

void
read_n(FILE* file, std::ranges::contiguous_range auto values, std::uint8_t bytes = sizeof(*values))
{
  // todo read together, optionally swap afterwards
  for (std::size_t i = 0; i < std::ranges::size(values); ++i)
    read(file, std::ranges::data(values)[i], bytes);
}

void
write_n(FILE* file, std::ranges::contiguous_range auto values, std::uint8_t bytes = sizeof(*values))
{
  for (std::size_t i = 0; i < std::ranges::size(values); ++i)
    write(file, std::ranges::data(values)[i], bytes);
}

void
write_n_times(FILE* file, auto value, std::size_t count, std::uint8_t bytes = sizeof(value))
{
  for (std::size_t i = 0; i < count; ++i)
    write(file, value, bytes);
}

/// Utility to identify the number
/// of bytes to use to store a value.
[[nodiscard]] constexpr std::uint8_t
bytes_small(std::uint64_t value) noexcept
{
  if (value == 0)
    return 0;
  const std::uint64_t bits = std::bit_width(value);
  const std::uint64_t bytes = (bits + 7) / 8;
  return static_cast<std::uint8_t>(bytes);
}

[[nodiscard]] constexpr std::uint8_t
bytes_fast(std::uint64_t value) noexcept
{
  return std::bit_ceil(bytes_small(value));
}

void
dispatch_uint(std::uint8_t bytes, auto&& f)
{
  // clang-format off
  switch (std::bit_ceil(bytes)) {
    case 1:  return f(std::uint8_t{});
    case 2:  return f(std::uint16_t{});
    case 4:  return f(std::uint32_t{});
    case 8:  return f(std::uint64_t{});
    default: throw std::runtime_error("unsupported byte width");
  }
  // clang-format on
}

}
