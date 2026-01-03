#pragma once

#include <kaspan/util/arithmetic.hpp>

#include <algorithm>
#include <bit>
#include <cstring>
#include <span>

namespace kaspan {

template<typename Derived, unsigned_concept T = u64>
class dense_unsigned_mixin
{
public:
  void fill(T value, u64 n)
  {
    for (u64 i = 0; i < n; ++i) {
      set(i, value);
    }
  }

  [[nodiscard]] auto get(u64 index) const -> T
  {
    T          result{};
    auto       result_view = std::as_writable_bytes(std::span{ &result, 1 });
    auto const data_view   = std::as_bytes(std::span{ derived()->data() + index * derived()->element_bytes(), derived()->element_bytes() });

    // ReSharper disable CppDFAUnreachableCode
    if (std::endian::native == std::endian::little) {
      if (derived()->endian() == std::endian::big) {                                       // rv = 03 02 01
        std::memcpy(result_view.data(), data_view.data(), derived()->element_bytes());     // bv = 03 02 01 00
        std::reverse(result_view.data(), result_view.data() + derived()->element_bytes()); // bv = 01 02 03 00
      } else {                                                                             // rv = 01 02 03
        std::memcpy(result_view.data(), data_view.data(), derived()->element_bytes());     // bv = 01 02 03 00
      }
    } else {
      if (derived()->endian() == std::endian::little) {                                // rv = 01 02 03
        std::memcpy(result_view.data(), data_view.data(), derived()->element_bytes()); // bv = 01 02 03 00
        std::reverse(result_view.data(), result_view.data() + sizeof(T));              // bv = 00 03 02 01
      } else {                                                                         // rv = 03 02 01
        auto const offset = sizeof(T) - derived()->element_bytes();
        std::memcpy(result_view.data() + offset, data_view.data(), derived()->element_bytes()); // bv = 00 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode
    return result;
  }

  void set(u64 index, T val)
  {
    auto value_view = std::as_writable_bytes(std::span{ &val, 1 });
    auto data_view  = std::as_writable_bytes(std::span{ derived()->data() + index * derived()->element_bytes(), derived()->element_bytes() });

    // ReSharper disable CppDFAUnreachableCode
    if (std::endian::native == std::endian::little) { // bv = 01 02 03 00
      if (derived()->endian() == std::endian::big) {
        std::memcpy(data_view.data(), value_view.data(), derived()->element_bytes());  // rv = 01 02 03
        std::reverse(data_view.data(), data_view.data() + derived()->element_bytes()); // rv = 03 02 01
      } else {
        std::memcpy(data_view.data(), value_view.data(), derived()->element_bytes()); // rv = 01 02 03
      }
    } else { // bv = 00 03 02 01
      if (derived()->endian() == std::endian::little) {
        std::reverse(value_view.data(), value_view.data() + sizeof(T));               // bv = 01 02 03 00
        std::memcpy(data_view.data(), value_view.data(), derived()->element_bytes()); // rv = 01 02 03
      } else {
        auto const offset = sizeof(T) - derived()->element_bytes();
        std::memcpy(data_view.data(), value_view.data() + offset, derived()->element_bytes()); // rv = 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode
  }

protected:
  ~dense_unsigned_mixin() = default;

private:
  auto derived() -> Derived*
  {
    return static_cast<Derived*>(this);
  }

  [[nodiscard]] auto derived() const -> Derived const*
  {
    return static_cast<Derived const*>(this);
  }
};

} // namespace kaspan
