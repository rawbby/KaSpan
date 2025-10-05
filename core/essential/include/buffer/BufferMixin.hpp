#pragma once

#include <cstddef>
#include <span>

template<class D, typename /* V */>
class BufferMixin
{
public:
  [[nodiscard]] bool empty() const
  {
    return static_cast<D const*>(this)->size() == 0;
  }

  [[nodiscard]] bool operator==(D const& rhs) const
  {
    return static_cast<D const*>(this)->data() == rhs.data();
  }

  [[nodiscard]] bool operator!=(D const& rhs) const
  {
    return static_cast<D const*>(this)->data() != rhs.data();
  }

  [[nodiscard]] auto byte_range() -> std::span<byte>
  {
    auto* byte_data = static_cast<byte*>(static_cast<D*>(this)->data());
    return { byte_data, static_cast<D*>(this)->bytes() };
  }

  [[nodiscard]] auto byte_range() const -> std::span<byte const>
  {
    auto* byte_data = static_cast<byte const*>(static_cast<D const*>(this)->data());
    return { byte_data, static_cast<D const*>(this)->bytes() };
  }
};

template<class D, typename V>
class ByteBufferMixin : public BufferMixin<D, V>
{
public:
  using BufferMixin<D, V>::byte_range;

  [[nodiscard]] auto byte_range(size_t index) -> std::span<byte>
  {
    auto* const byte_data     = static_cast<byte*>(static_cast<D*>(this)->data());
    auto const  element_bytes = static_cast<D*>(this)->element_bytes();
    auto const  offset        = index * element_bytes;
    return { byte_data + offset, element_bytes };
  }

  [[nodiscard]] auto byte_range(size_t index) const -> std::span<byte const>
  {
    auto const* const byte_data     = static_cast<byte const*>(static_cast<D*>(this)->data());
    auto const        element_bytes = static_cast<D const*>(this)->element_bytes();
    auto const        offset        = index * element_bytes;
    return { byte_data + offset, element_bytes };
  }

  [[nodiscard]] auto byte_range(size_t index, size_t length) -> std::span<byte>
  {
    auto* const byte_data     = static_cast<byte*>(static_cast<D*>(this)->data());
    auto const  element_bytes = static_cast<D*>(this)->element_bytes();
    auto const  offset        = index * element_bytes;
    return { byte_data + offset, length * element_bytes };
  }

  [[nodiscard]] auto byte_range(size_t index, size_t length) const -> std::span<byte const>
  {
    auto const* const byte_data     = static_cast<byte const*>(static_cast<D const*>(this)->data());
    auto const        element_bytes = static_cast<D const*>(this)->element_bytes();
    auto const        offset        = index * element_bytes;
    return { byte_data + offset, length * element_bytes };
  }

  [[nodiscard]] auto bytes() const -> size_t
  {
    auto const element_bytes = static_cast<D const*>(this)->element_bytes();
    return element_bytes * static_cast<D const*>(this)->size();
  }
};

template<typename D, typename V>
class IndirectBufferMixin : public BufferMixin<D, V>
{};

template<typename Derived, typename value_type>
class IndirectByteBufferMixin
  : public ByteBufferMixin<Derived, value_type>
{
};

template<typename D, typename V>
class DirectBufferMixin : public IndirectByteBufferMixin<D, V>
{
public:
  [[nodiscard]] V get(size_t index) const
  {
    auto const* const element_data = static_cast<V const*>(static_cast<D const*>(this)->data());
    return element_data[index];
  }

  void set(size_t index, V value)
  {
    auto* const element_data = static_cast<V*>(static_cast<D*>(this)->data());
    element_data[index]      = value;
  }

  [[nodiscard]] V& operator[](size_t index)
  {
    auto* const element_data = static_cast<V*>(static_cast<D*>(this)->data());
    return element_data[index];
  }

  [[nodiscard]] V const& operator[](size_t index) const
  {
    auto const* const element_data = static_cast<V const*>(static_cast<D const*>(this)->data());
    return element_data[index];
  }

  [[nodiscard]] auto range() -> std::span<V>
  {
    auto* const element_data = static_cast<V*>(static_cast<D*>(this)->data());
    return { element_data, static_cast<D*>(this)->size() };
  }

  [[nodiscard]] auto range() const -> std::span<V const>
  {
    auto const* const element_data = static_cast<V const*>(static_cast<D const*>(this)->data());
    return { element_data, static_cast<D const*>(this)->size() };
  }

  [[nodiscard]] auto range(size_t index, size_t length) -> std::span<V>
  {
    auto* const element_data = static_cast<V*>(static_cast<D*>(this)->data());
    return { element_data + index, length };
  }

  [[nodiscard]] auto range(size_t index, size_t length) const -> std::span<V const>
  {
    auto const* const element_data = static_cast<V const*>(static_cast<D const*>(this)->data());
    return { element_data + index, length };
  }

  // ReSharper disable once CppMemberFunctionMayBeStatic
  [[nodiscard]] auto element_bytes() const -> size_t
  {
    return sizeof(V);
  }
};
