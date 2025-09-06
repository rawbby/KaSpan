// ReSharper disable CppRedundantTemplateArguments

#include <buffer/Buffer.hpp>
#include <util/Arithmetic.hpp>

int
main()
{
  static_assert(DirectBufferConcept<Buffer>);
  static_assert(IndirectByteBufferConcept<Buffer>);
  static_assert(BufferConcept<Buffer>);

  static_assert(DirectBufferConcept<FileBuffer>);
  static_assert(IndirectByteBufferConcept<FileBuffer>);
  static_assert(BufferConcept<FileBuffer>);

  static_assert(DirectBufferConcept<IntBuffer<i8>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<i8>>);
  static_assert(BufferConcept<IntBuffer<i8>>);

  static_assert(DirectBufferConcept<IntBuffer<i16>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<i16>>);
  static_assert(BufferConcept<IntBuffer<i16>>);

  static_assert(DirectBufferConcept<IntBuffer<i32>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<i32>>);
  static_assert(BufferConcept<IntBuffer<i32>>);

  static_assert(DirectBufferConcept<IntBuffer<i64>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<i64>>);
  static_assert(BufferConcept<IntBuffer<i64>>);

  static_assert(DirectBufferConcept<IntBuffer<u8>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<u8>>);
  static_assert(BufferConcept<IntBuffer<u8>>);

  static_assert(DirectBufferConcept<IntBuffer<u16>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<u16>>);
  static_assert(BufferConcept<IntBuffer<u16>>);

  static_assert(DirectBufferConcept<IntBuffer<u32>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<u32>>);
  static_assert(BufferConcept<IntBuffer<u32>>);

  static_assert(DirectBufferConcept<IntBuffer<u64>>);
  static_assert(IndirectByteBufferConcept<IntBuffer<u64>>);
  static_assert(BufferConcept<IntBuffer<u64>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u8, Buffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u8, Buffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u16, Buffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u16, Buffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u32, Buffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u32, Buffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u64, Buffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u64, Buffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u8, FileBuffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u8, FileBuffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u16, FileBuffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u16, FileBuffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u32, FileBuffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u32, FileBuffer>>);

  static_assert(IndirectByteBufferConcept<CompressedUnsignedBuffer<u64, FileBuffer>>);
  static_assert(BufferConcept<CompressedUnsignedBuffer<u64, FileBuffer>>);

  static_assert(IndirectBufferConcept<BitBuffer>);
  static_assert(BufferConcept<BitBuffer>);
}
