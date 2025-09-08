#include "buffer/Accessor.hpp"

#include <buffer/Buffer.hpp>
#include <util/Arithmetic.hpp>

int
main()
{
  static_assert(BufferConcept<Buffer>);
  static_assert(BufferConcept<FileBuffer>);

  static_assert(IterableAccessorConcept<BoolAccessor>);
  static_assert(IterableAccessorConcept<ByteAccessor>);
  static_assert(IterableAccessorConcept<I8Accessor>);
  static_assert(IterableAccessorConcept<I16Accessor>);
  static_assert(IterableAccessorConcept<I32Accessor>);
  static_assert(IterableAccessorConcept<I64Accessor>);
  static_assert(IterableAccessorConcept<U8Accessor>);
  static_assert(IterableAccessorConcept<U16Accessor>);
  static_assert(IterableAccessorConcept<U32Accessor>);
  static_assert(IterableAccessorConcept<U64Accessor>);
  static_assert(requires(FileBuffer fb, I16Accessor a, size_t i, i16 v) {
    { a.get(fb, i) } -> std::convertible_to<i16>;
    { a.set(fb, i, v) } -> std::same_as<void>;
  });
  static_assert(requires(Buffer b, I16Accessor a, size_t i, i16 v) {
    { a.get(b, i) } -> std::convertible_to<i16>;
    { a.set(b, i, v) } -> std::same_as<void>;
  });

  static_assert(AccessorConcept<CU16Accessor>);
  static_assert(AccessorConcept<CU32Accessor>);
  static_assert(AccessorConcept<CU64Accessor>);
  static_assert(requires(FileBuffer fb, CU16Accessor a, size_t i, u16 v) {
    { a.get(fb, i) } -> std::convertible_to<u16>;
    { a.set(fb, i, v) } -> std::same_as<void>;
  });
  static_assert(requires(Buffer b, CU16Accessor a, size_t i, u16 v) {
    { a.get(b, i) } -> std::convertible_to<u16>;
    { a.set(b, i, v) } -> std::same_as<void>;
  });

  static_assert(AccessorConcept<BitAccessor>);
  static_assert(requires(FileBuffer fb, BitAccessor a, size_t i, bool v) {
    { a.get(fb, i) } -> std::convertible_to<bool>;
    { a.set(fb, i, v) } -> std::same_as<void>;
  });
  static_assert(requires(Buffer b, BitAccessor a, size_t i, bool v) {
    { a.get(b, i) } -> std::convertible_to<bool>;
    { a.set(b, i, v) } -> std::same_as<void>;
  });
}
