#pragma once

#include <concepts>
#include <cstddef>
#include <span>

#include <util/Arithmetic.hpp>

/**
 * \brief concept for owning, continuous and page aligned buffers.
 */
template<class B>
concept BufferConcept =
  std::is_default_constructible_v<B> and
  std::is_move_constructible_v<B> and
  std::is_move_assignable_v<B> and

  requires(B b, B const cb, typename B::value_type v) {
    requires std::is_trivially_copyable_v<decltype(v)>;

    { cb.bytes() } -> std::convertible_to<size_t>; // allocated bytes
    { cb.size() } -> std::convertible_to<size_t>;  // number of elements

    // memory pointer
    { b.data() } -> std::convertible_to<void*>;
    { cb.data() } -> std::convertible_to<void const*>;
    { b.byte_range() } -> std::convertible_to<std::span<byte>>;
    { cb.byte_range() } -> std::convertible_to<std::span<byte const>>;

    // utilities
    { cb.empty() } -> std::convertible_to<bool>;
    { cb == cb } -> std::convertible_to<bool>;
    { cb != cb } -> std::convertible_to<bool>;

    // tags
    { B::compressed } -> std::convertible_to<bool>;
  };

/**
 * \brief concept for buffers where stored elements use n bytes.
 *
 * example: int-buffers use four bytes per element.
 * counterexample: bit-buffers use less than one byte per element.
 */
template<class B>
concept ByteBufferConcept =
  BufferConcept<B> and
  requires(B b, B const cb, size_t index, size_t length) {
    { cb.element_bytes() } -> std::convertible_to<size_t>;
    { b.byte_range(index) } -> std::convertible_to<std::span<byte>>;
    { cb.byte_range(index) } -> std::convertible_to<std::span<byte const>>;
    { b.byte_range(index, length) } -> std::convertible_to<std::span<byte>>;
    { cb.byte_range(index, length) } -> std::convertible_to<std::span<byte const>>;
  };

/**
 * \brief concept for indirect buffers.
 *
 * example: compressed data, where the queries require compression and decompression.
 */
template<class B>
concept IndirectBufferConcept =
  BufferConcept<B> and
  requires(B b, B const cb, size_t index, typename B::value_type v) {
    { cb.get(index) } -> std::convertible_to<decltype(v)>;
    { b.set(index, v) } -> std::same_as<void>;
  };

/**
 * \brief concept for indirect byte buffers.
 */
template<class B>
concept IndirectByteBufferConcept = ByteBufferConcept<B> and IndirectBufferConcept<B>;

/**
 * \brief concept for direct buffers.
 *
 * values dont need translation. they are stored ready to use in memory.
 */
template<class B>
concept DirectBufferConcept =
  IndirectByteBufferConcept<B> and
  requires(B b, B const cb, size_t index, size_t length, typename B::value_type v) {
    { b[index] } -> std::convertible_to<decltype(v)&>;
    { cb[index] } -> std::convertible_to<decltype(v) const&>;

    { b.range() } -> std::convertible_to<std::span<decltype(v)>>;
    { cb.range() } -> std::convertible_to<std::span<decltype(v) const>>;
    { b.range(index, length) } -> std::convertible_to<std::span<decltype(v)>>;
    { cb.range(index, length) } -> std::convertible_to<std::span<decltype(v) const>>;
  };
