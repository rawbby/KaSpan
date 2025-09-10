#pragma once

#include <buffer/Buffer.hpp>
#include <util/Result.hpp>

namespace internal {

inline auto
memory_copy_bytes(std::span<byte> dst, std::span<byte const> src) -> VoidResult
{
  std::memcpy(dst.data(), src.data(), std::min(dst.size_bytes(), src.size_bytes()));
  return VoidResult::success();
}

template<IntConcept Int>
auto
memory_copy_ints(std::span<Int> dst, std::span<Int const> src) -> VoidResult
{
  std::memcpy(dst.data(), src.data(), std::min(dst.size_bytes(), src.size_bytes()));
  return VoidResult::success();
}

template<IndirectBufferConcept Dst, IndirectBufferConcept Src>
  requires std::convertible_to<typename Src::value_type, typename Dst::value_type>
auto
copy_indirect(Dst& dst, Src const& src) -> VoidResult
{
  auto const n = std::min(dst.size(), src.size());
  for (size_t i = 0; i < n; ++i)
    dst.set(i, src.get(i));
  return VoidResult::success();
}

} // namespace internal

inline auto
memory_copy(std::span<byte> dst, std::span<byte const> src) -> VoidResult
{
  return internal::memory_copy_bytes(dst, src);
}

template<IntConcept Int>
auto
memory_copy(std::span<Int> dst, std::span<Int const> src) -> VoidResult
{
  return internal::memory_copy_ints(dst, src);
}

template<BufferConcept Dst, BufferConcept Src>
auto
memory_copy(Dst& dst, Src const& src) -> VoidResult
{
  return internal::memory_copy_bytes(dst.byte_range(), src.byte_range());
}

template<IndirectBufferConcept Dst, IndirectBufferConcept Src>
  requires std::convertible_to<typename Src::value_type, typename Dst::value_type>
auto
copy(Dst& dst, Src const& src) -> VoidResult
{
  return internal::copy_indirect(dst, src);
}

template<DirectBufferConcept Dst, DirectBufferConcept Src>
  requires std::same_as<typename Src::value_type, typename Dst::value_type>
auto
copy(Dst& dst, Src const& src) -> VoidResult
{
  return internal::memory_copy_bytes(dst, src);
}

template<UnsignedConcept DstUnsigned, DirectBufferConcept DstBacked, UnsignedConcept SrcUnsigned, DirectBufferConcept SrcBacked>
auto
copy(CompressedUnsignedBuffer<DstUnsigned, DstBacked>& dst, CompressedUnsignedBuffer<SrcUnsigned, SrcBacked> const& src) -> VoidResult
{
  if (dst.element_bytes() == src.element_bytes()) {
    return internal::memory_copy_bytes(dst.byte_range(), src.byte_range());
  }
  return internal::copy_indirect(dst, src);
}
