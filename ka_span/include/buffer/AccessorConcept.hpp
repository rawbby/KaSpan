#pragma once

#include <util/Arithmetic.hpp>
#include <util/Util.hpp>

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <span>

// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppNonExplicitConversionOperator

/// context to raw memory
template<class A>
concept AccessorConcept =

  std::equality_comparable<A>

  and

  requires {
    std::is_default_constructible_v<A>;
    std::is_move_constructible_v<A>;
    std::is_move_assignable_v<A>;
    std::is_copy_constructible_v<A>;
    std::is_copy_assignable_v<A>;

    typename A::value_type;
    std::is_default_constructible_v<typename A::value_type>;
    std::is_move_constructible_v<typename A::value_type>;
    std::is_move_assignable_v<typename A::value_type>;
    std::is_copy_constructible_v<typename A::value_type>;
    std::is_copy_assignable_v<typename A::value_type>;
  }

  and

  requires(A const ca, void* b, void const* cb, size_t index, typename A::value_type v) {
    /* Element access */
    { ca.get(cb, index) } -> std::convertible_to<decltype(v)>;
    { ca.set(b, index, v) } -> std::same_as<void>;

    /* Observers */
    { ca.empty() } -> std::convertible_to<bool>;
    { ca.size() } -> std::convertible_to<size_t>;
    { ca.size_bytes() } -> std::convertible_to<size_t>;
  };

/// context to raw memory
template<class A>
concept IterableAccessorConcept =

  AccessorConcept<A>

  and

  requires(A const ca, void* b, void const* cb, size_t index, typename A::value_type v) {
    /* Iterators */
    { ca.begin(b) } -> std::random_access_iterator<>;
    { ca.begin(cb) } -> std::random_access_iterator<>;
    { ca.cbegin(cb) } -> std::random_access_iterator<>;
    { ca.end(b) } -> std::random_access_iterator<>;
    { ca.end(cb) } -> std::random_access_iterator<>;
    { ca.cend(cb) } -> std::random_access_iterator<>;
  };

template<typename T>
class Accessor final
{
public:
  static constexpr size_t alignment = alignof(T);

  using value_type = T;

  constexpr Accessor() noexcept  = default;
  constexpr ~Accessor() noexcept = default;

  explicit constexpr Accessor(size_t size) noexcept
    : size_(size)
  {
  }

  constexpr Accessor(Accessor&&) noexcept      = default;
  constexpr Accessor(Accessor const&) noexcept = default;

  constexpr auto operator=(Accessor&&) noexcept -> Accessor&      = default;
  constexpr auto operator=(Accessor const&) noexcept -> Accessor& = default;

  // clang-format off
  [[nodiscard]] constexpr auto begin(void* b) const noexcept -> T* { return static_cast<T*>(b); }
  [[nodiscard]] constexpr auto begin(void const* cb) const noexcept -> T const* { return static_cast<T const*>(cb); }
  [[nodiscard]] constexpr auto cbegin(void const* cb) const noexcept -> T const* { return static_cast<T const*>(cb); }
  [[nodiscard]] constexpr auto end(void* b) const noexcept -> T* { return static_cast<T*>(b) + size_; }
  [[nodiscard]] constexpr auto end(void const* cb) const noexcept -> T const* { return static_cast<T const*>(cb) + size_; }
  [[nodiscard]] constexpr auto cend(void const* cb) const noexcept -> T const* { return static_cast<T const*>(cb) + size_; }
  [[nodiscard]] constexpr auto get(void const* cb, size_t i) const noexcept -> T const& { return *(static_cast<T const*>(cb) + i); }
  constexpr void set(void* b, size_t i, T const& v) const noexcept { *(static_cast<T*>(b) + i) = v; }
  [[nodiscard]] constexpr auto size() const noexcept -> size_t { return size_; }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return size_ == 0; }
  [[nodiscard]] constexpr auto size_bytes() const noexcept -> size_t { return size_ * sizeof(T); }
  [[nodiscard]] constexpr auto operator==(Accessor const& rhs) const noexcept -> bool { return size_ == rhs.size_; }
  [[nodiscard]] constexpr auto operator!=(Accessor const& rhs) const noexcept -> bool { return size_ != rhs.size_; }
  [[nodiscard]] constexpr auto range(void* b) const noexcept -> std::span<T> { return { static_cast<T*>(b), size_ }; }
  [[nodiscard]] constexpr auto range(void* b, size_t n) const noexcept -> std::span<T> { return { static_cast<T*>(b), n }; }
  [[nodiscard]] constexpr auto range(void* b, size_t i, size_t n) const noexcept -> std::span<T> { return { static_cast<T*>(b) + i, n }; }
  [[nodiscard]] constexpr auto range(void const* cb) const noexcept -> std::span<T const> { return { static_cast<T const*>(cb), size_ }; }
  [[nodiscard]] constexpr auto range(void const* cb, size_t n) const noexcept -> std::span<T const> { return { static_cast<T const*>(cb), n }; }
  [[nodiscard]] constexpr auto range(void const* cb, size_t i, size_t n) const noexcept -> std::span<T const> { return { static_cast<T const*>(cb) + i, n }; }
  // clang-format on

private:
  size_t size_ = 0;
};

template<UnsignedConcept T>
class CompressedUnsignedAccessor final
{
public:
  static constexpr size_t alignment = 1;

  using value_type = T;

  constexpr CompressedUnsignedAccessor() noexcept  = default;
  constexpr ~CompressedUnsignedAccessor() noexcept = default;

  constexpr CompressedUnsignedAccessor(size_t size, size_t element_size) noexcept
    : size_(size)
    , element_size_(element_size)
  {
  }

  constexpr CompressedUnsignedAccessor(CompressedUnsignedAccessor&&) noexcept      = default;
  constexpr CompressedUnsignedAccessor(CompressedUnsignedAccessor const&) noexcept = default;

  constexpr auto operator=(CompressedUnsignedAccessor&&) noexcept -> CompressedUnsignedAccessor&      = default;
  constexpr auto operator=(CompressedUnsignedAccessor const&) noexcept -> CompressedUnsignedAccessor& = default;

  constexpr void translate_endian(void* b, std::endian from_endian, std::endian to_endian = std::endian::native) const noexcept
  {
    if (from_endian != to_endian) {
      auto const end = size_bytes();
      for (size_t it = 0; it < end; it += element_size_) {
        auto element = std::span{ static_cast<byte*>(b) + it, element_size_ };
        std::reverse(element.data(), element.data() + element_size_);
      }
    }
  }

  [[nodiscard]] constexpr auto get(void const* cb, size_t i, std::endian cb_endian = std::endian::native) const noexcept -> T
  {
    value_type result{};
    auto       buffer_view = std::as_writable_bytes(std::span{ &result, 1 });
    auto const result_view = std::span{ static_cast<byte const*>(cb) + i * element_size_, element_size_ };

    // ReSharper disable CppDFAUnreachableCode
    if constexpr (std::endian::native == std::endian::little) {
      if (cb_endian == std::endian::big) {                                    // rv = 03 02 01
        std::memcpy(buffer_view.data(), result_view.data(), element_size_);   // bv = 03 02 01 00
        std::reverse(buffer_view.data(), buffer_view.data() + element_size_); // bv = 01 02 03 00
      } else {                                                                // rv = 01 02 03
        std::memcpy(buffer_view.data(), result_view.data(), element_size_);   // bv = 01 02 03 00
      }
    } else {
      if (cb_endian == std::endian::little) {                                      // rv = 01 02 03
        std::memcpy(buffer_view.data(), result_view.data(), element_size_);        // bv = 01 02 03 00
        std::reverse(buffer_view.data(), buffer_view.data() + sizeof(value_type)); // bv = 00 03 02 01
      } else {                                                                     // rv = 03 02 01
        auto const offset = sizeof(value_type) - element_size_;
        std::memcpy(buffer_view.data() + offset, result_view.data(), element_size_); // bv = 00 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode

    return result;
  }

  constexpr void set(void* b, size_t i, T v, std::endian b_endian = std::endian::native) const noexcept
  {
    auto buffer_view = std::as_writable_bytes(std::span{ &v, 1 });
    auto result_view = std::span{ static_cast<byte*>(b) + i * element_size_, element_size_ };

    // ReSharper disable CppDFAUnreachableCode
    if constexpr (std::endian::native == std::endian::little) { // bv = 01 02 03 00
      if (b_endian == std::endian::big) {
        std::memcpy(result_view.data(), buffer_view.data(), element_size_);   // rv = 01 02 03
        std::reverse(result_view.data(), result_view.data() + element_size_); // rv = 03 02 01
      } else {
        std::memcpy(result_view.data(), buffer_view.data(), element_size_); // rv = 01 02 03
      }
    } else { // bv = 00 03 02 01
      if (b_endian == std::endian::little) {
        std::reverse(buffer_view.data(), buffer_view.data() + sizeof(value_type)); // bv = 01 02 03 00
        std::memcpy(result_view.data(), buffer_view.data(), element_size_);        // rv = 01 02 03
      } else {
        auto const offset = sizeof(value_type) - element_size_;
        std::memcpy(result_view.data(), buffer_view.data() + offset, element_size_); // rv = 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode
  }

  // clang-format off
  [[nodiscard]] constexpr auto size() const noexcept -> size_t { return size_; }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return size_ == 0; }
  [[nodiscard]] constexpr auto size_bytes() const noexcept -> size_t { return size_ * element_size_; }
  [[nodiscard]] constexpr auto operator==(CompressedUnsignedAccessor const& rhs) const noexcept -> bool { return size_ == rhs.size_ and element_size_ == rhs.element_size_; }
  [[nodiscard]] constexpr auto operator!=(CompressedUnsignedAccessor const& rhs) const noexcept -> bool { return size_ != rhs.size_ and element_size_ != rhs.element_size_; }
  // clang-format on

private:
  size_t size_         = 0;
  size_t element_size_ = 0;
};

class BitAccessor final
{
public:
  static constexpr size_t alignment = 8;

  using value_type = bool;

  constexpr BitAccessor() noexcept  = default;
  constexpr ~BitAccessor() noexcept = default;

  explicit constexpr BitAccessor(size_t size) noexcept
    : size_(size)
  {
  }

  constexpr BitAccessor(BitAccessor const&) noexcept = default;
  constexpr BitAccessor(BitAccessor&& rhs) noexcept  = default;

  constexpr auto operator=(BitAccessor const&) noexcept -> BitAccessor& = default;
  constexpr auto operator=(BitAccessor&& rhs) noexcept -> BitAccessor&  = default;

  [[nodiscard]] constexpr auto get(void const* cb, size_t bit_index) const noexcept -> bool
  {
    auto const [index, mask] = index_mask(bit_index);
    return (static_cast<u64 const*>(cb)[index] & mask) != 0;
  }

  constexpr void set(void* b, size_t bit_index, bool value) const noexcept
  {
    auto const [index, mask] = index_mask(bit_index);
    auto const value_mask    = -static_cast<u64>(value);

    auto field = static_cast<u64*>(b)[index]; // single load
    field &= ~mask;                           // unconditional unset
    field |= value_mask & mask;               // conditional set
    static_cast<u64*>(b)[index] = field;      // single store
  }

  constexpr void set(void* b, size_t bit_index) const noexcept
  {
    auto const [index, mask] = index_mask(bit_index);
    static_cast<u64*>(b)[index] |= mask;
  }

  constexpr void
  unset(void* b, size_t bit_index) const noexcept
  {
    auto const [index, mask] = index_mask(bit_index);
    static_cast<u64*>(b)[index] &= ~mask;
  }

  // clang-format off
  [[nodiscard]] constexpr auto size() const noexcept -> size_t { return size_; }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return size_ == 0; }
  [[nodiscard]] constexpr auto size_bytes() const noexcept -> size_t { return (size_ + 63) / 64; }
  [[nodiscard]] constexpr auto operator==(BitAccessor const& rhs) const noexcept -> bool { return size_ == rhs.size_; }
  [[nodiscard]] constexpr auto operator!=(BitAccessor const& rhs) const noexcept -> bool { return size_ != rhs.size_; }
  // clang-format on

private:
  [[nodiscard]] static constexpr auto index_mask(size_t bit_index) noexcept -> std::pair<size_t, u64>
  {
    auto const index = bit_index >> 6;
    auto const mask  = static_cast<u64>(1) << (bit_index & static_cast<size_t>(0b111111));
    return { index, mask };
  }

private:
  size_t size_ = 0;
};

using BoolAccessor = Accessor<bool>;
using ByteAccessor = Accessor<byte>;
using I8Accessor   = Accessor<i8>;
using I16Accessor  = Accessor<i16>;
using I32Accessor  = Accessor<i32>;
using I64Accessor  = Accessor<i64>;
using U8Accessor   = Accessor<u8>;
using U16Accessor  = Accessor<u16>;
using U32Accessor  = Accessor<u32>;
using U64Accessor  = Accessor<u64>;

using CU16Accessor = CompressedUnsignedAccessor<u16>;
using CU32Accessor = CompressedUnsignedAccessor<u32>;
using CU64Accessor = CompressedUnsignedAccessor<u64>;

// ReSharper restore CppNonExplicitConversionOperator
// ReSharper restore CppMemberFunctionMayBeStatic
