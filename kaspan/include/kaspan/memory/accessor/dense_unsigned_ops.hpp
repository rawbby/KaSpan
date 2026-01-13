#pragma once

#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <algorithm>
#include <bit>
#include <cstring>
#include <span>

namespace kaspan {

struct dense_unsigned_ops
{
  template<unsigned_concept T>
  static auto get(std::byte const* data, u64 index, u8 element_bytes, std::endian endian) -> T
  {
    T          result{};
    auto       result_view = std::as_writable_bytes(std::span{ &result, 1 });
    auto const data_view   = std::as_bytes(std::span{ data + index * element_bytes, element_bytes });

    if (std::endian::native == std::endian::little) {
      if (endian == std::endian::big) {
        std::memcpy(result_view.data(), data_view.data(), element_bytes);
        std::reverse(result_view.data(), result_view.data() + element_bytes);
      } else {
        std::memcpy(result_view.data(), data_view.data(), element_bytes);
      }
    } else {
      if (endian == std::endian::little) {
        std::memcpy(result_view.data(), data_view.data(), element_bytes);
        std::reverse(result_view.data(), result_view.data() + sizeof(T));
      } else {
        auto const offset = sizeof(T) - element_bytes;
        std::memcpy(result_view.data() + offset, data_view.data(), element_bytes);
      }
    }
    return result;
  }

  template<unsigned_concept T>
  static void set(std::byte* data, u64 index, u8 element_bytes, std::endian endian, T val)
  {
    auto value_view = std::as_writable_bytes(std::span{ &val, 1 });
    auto data_view  = std::as_writable_bytes(std::span{ data + index * element_bytes, element_bytes });

    if (std::endian::native == std::endian::little) {
      if (endian == std::endian::big) {
        std::memcpy(data_view.data(), value_view.data(), element_bytes);
        std::reverse(data_view.data(), data_view.data() + element_bytes);
      } else {
        std::memcpy(data_view.data(), value_view.data(), element_bytes);
      }
    } else {
      if (endian == std::endian::little) {
        std::reverse(value_view.data(), value_view.data() + sizeof(T));
        std::memcpy(data_view.data(), value_view.data(), element_bytes);
      } else {
        auto const offset = sizeof(T) - element_bytes;
        std::memcpy(data_view.data(), value_view.data() + offset, element_bytes);
      }
    }
  }

  template<unsigned_concept T>
  static void fill(std::byte* data, u64 n, u8 element_bytes, std::endian endian, T value)
  {
    for (u64 i = 0; i < n; ++i) {
      set(data, i, element_bytes, endian, value);
    }
  }
};

} // namespace kaspan
