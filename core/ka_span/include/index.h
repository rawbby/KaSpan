#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>
#include <variant>

namespace ka_span {

template<typename T>
concept is_index = std::is_same_v<std::remove_cvref_t<T>, std::uint8_t> or
                   std::is_same_v<std::remove_cvref_t<T>, std::uint16_t> or
                   std::is_same_v<std::remove_cvref_t<T>, std::uint32_t> or
                   std::is_same_v<std::remove_cvref_t<T>, std::uint64_t>;

constexpr std::variant<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>
index_from_num(is_index auto num)
{
  if constexpr (sizeof(num) > sizeof(std::uint32_t))
    if (num > std::numeric_limits<std::uint32_t>::max())
      return static_cast<std::uint64_t>(num);

  if constexpr (sizeof(num) > sizeof(std::uint16_t))
    if (num > std::numeric_limits<std::uint16_t>::max())
      return static_cast<std::uint32_t>(num);

  if constexpr (sizeof(num) > sizeof(std::uint8_t))
    if (num > std::numeric_limits<std::uint8_t>::max())
      return static_cast<std::uint16_t>(num);

  return static_cast<std::uint8_t>(num);
}

constexpr auto
dispatch_index_from_num(auto&& fn, is_index auto num)
{
  return std::visit(
    [&](is_index auto index) {
      return std::forward<decltype(fn)>(fn)(index);
    },
    index_from_num(num));
}

constexpr auto
dispatch_indices_from_nums(auto&& fn)
{
  return std::forward<decltype(fn)>(fn)();
}

constexpr auto
dispatch_indices_from_nums(auto&& fn, is_index auto num, is_index auto... nums)
{
  return dispatch_index_from_num(
    [&](is_index auto index) {
      return dispatch_indices_from_nums(
        [&](is_index auto... indices) {
          return std::forward<decltype(fn)>(fn)(index, indices...);
        },
        nums...);
    },
    num);
}

constexpr std::uint8_t
index_size_from_num(is_index auto num)
{
  return dispatch_indices_from_nums(
    [&]([[maybe_unused]] auto index) {
      return static_cast<std::uint8_t>(sizeof(index));
    },
    num);
}

}
