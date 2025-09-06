#pragma once

#include <buffer/BufferConcept.hpp>
#include <buffer/BufferMixin.hpp>
#include <util/Arithmetic.hpp>
#include <util/Util.hpp>

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <span>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class Buffer : public DirectBufferMixin<Buffer, byte>
{
public:
  using value_type = byte;

  Buffer() = default;
  ~Buffer()
  {
    if (data_ != nullptr) {
      std::free(data_);
      data_ = nullptr;
      size_ = 0;
    }
  }

  Buffer(Buffer const&) = delete;
  Buffer(Buffer&& rhs) noexcept
  {
    std::swap(data_, rhs.data_);
    std::swap(size_, rhs.size_);
  }

  auto operator=(Buffer const&) -> Buffer& = delete;
  auto operator=(Buffer&& rhs) noexcept -> Buffer&
  {
    if (this != &rhs) {
      std::swap(data_, rhs.data_);
      std::swap(size_, rhs.size_);
    }
    return *this;
  }

  [[nodiscard]] static auto create(size_t size) -> Result<Buffer>
  {
    Buffer result{};
    if (size == 0)
      return result;
    RESULT_TRY(result.data_, page_aligned_alloc(size));
    result.size_ = size;
    return result;
  }

  [[nodiscard]] static auto zeroes(size_t size) -> Result<Buffer>
  {
    RESULT_TRY(auto result, create(size));
    std::memset(result.data(), 0, result.bytes());
    return result;
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto bytes() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_;
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_;
  }

private:
  void*  data_ = nullptr;
  size_t size_ = 0;
};

class FileBuffer : public DirectBufferMixin<FileBuffer, byte>
{
public:
  using value_type = byte;

  FileBuffer() = default;
  ~FileBuffer()
  {
    if (data_ && data_ != MAP_FAILED) {
      munmap(data_, size_);
      data_ = nullptr;
      size_ = 0;
    }
    if (fd_ != -1) {
      close(fd_);
      fd_ = -1;
    }
  }

  FileBuffer(FileBuffer const&) = delete;
  FileBuffer(FileBuffer&& rhs) noexcept
  {
    std::swap(fd_, rhs.fd_);
    std::swap(data_, rhs.data_);
    std::swap(size_, rhs.size_);
  }

  auto operator=(FileBuffer const&) -> FileBuffer& = delete;
  auto operator=(FileBuffer&& rhs) noexcept -> FileBuffer&
  {
    if (this != &rhs) {
      std::swap(fd_, rhs.fd_);
      std::swap(data_, rhs.data_);
      std::swap(size_, rhs.size_);
    }
    return *this;
  }

  [[nodiscard]] static auto create_r(char const* file, size_t size) -> Result<FileBuffer>
  {
    FileBuffer b{};
    if (size == 0)
      return b;

    b.fd_ = open(file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ASSERT_TRY(b.fd_ != -1, IO_ERROR);
    ASSERT_TRY(file_size_fd(b.fd_) == size, IO_ERROR);

    b.data_ = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, b.fd_, 0);
    ASSERT_TRY(b.data_ != MAP_FAILED, MEMORY_MAPPING_ERROR);

    b.size_ = size;
    return b;
  }

  [[nodiscard]] static auto create_w(char const* file, size_t size, bool allocate) -> Result<FileBuffer>
  {
    constexpr auto oflags       = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr auto alloc_oflags = oflags | O_CREAT | O_EXCL;

    FileBuffer b{};
    if (size == 0)
      return b;

    if (allocate) {
      b.fd_ = open(file, alloc_oflags, 0600);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(ftruncate(b.fd_, static_cast<off_t>(size)) == 0, IO_ERROR);
    } else {
      b.fd_ = open(file, oflags);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(file_size_fd(b.fd_) == size, IO_ERROR);
    }

    b.data_ = mmap(nullptr, size, PROT_WRITE, MAP_SHARED, b.fd_, 0);
    ASSERT_TRY(b.data_ != MAP_FAILED, MEMORY_MAPPING_ERROR);

    b.size_ = size;
    return b;
  }

  [[nodiscard]] static auto create_rw(char const* file, size_t size, bool allocate) -> Result<FileBuffer>
  {
    constexpr auto oflags       = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr auto alloc_oflags = oflags | O_CREAT | O_EXCL;

    FileBuffer b{};
    if (size == 0)
      return b;

    if (allocate) {
      b.fd_ = open(file, alloc_oflags, 0600);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(ftruncate(b.fd_, static_cast<off_t>(size)) == 0, IO_ERROR);
    } else {
      b.fd_ = open(file, oflags);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(file_size_fd(b.fd_) == size, IO_ERROR);
    }

    b.data_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, b.fd_, 0);
    ASSERT_TRY(b.data_ != MAP_FAILED, MEMORY_MAPPING_ERROR);

    b.size_ = size;
    return b;
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto bytes() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_;
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_;
  }

private:
  int    fd_   = -1;
  void*  data_ = nullptr;
  size_t size_ = 0;
};

template<IntConcept I>
class IntBuffer : public DirectBufferMixin<IntBuffer<I>, I>
{
public:
  using value_type = I;

  IntBuffer() = default;
  ~IntBuffer()
  {
    if (data_ != nullptr) {
      std::free(data_);
      data_ = nullptr;
      size_ = 0;
    }
  }

  IntBuffer(IntBuffer const&) = delete;
  IntBuffer(IntBuffer&& rhs) noexcept
  {
    std::swap(data_, rhs.data_);
    std::swap(size_, rhs.size_);
  }

  auto operator=(IntBuffer const&) -> IntBuffer& = delete;
  auto operator=(IntBuffer&& rhs) noexcept -> IntBuffer&
  {
    if (this != &rhs) {
      std::swap(data_, rhs.data_);
      std::swap(size_, rhs.size_);
    }
    return *this;
  }

  [[nodiscard]] static auto create(size_t size) -> Result<IntBuffer>
  {
    IntBuffer result{};
    if (size == 0)
      return result;
    RESULT_TRY(result.data_, page_aligned_alloc(size * sizeof(I)));
    result.size_ = size;
    return result;
  }

  [[nodiscard]] static auto zeroes(size_t size) -> Result<IntBuffer>
  {
    RESULT_TRY(auto result, create(size));
    std::memset(result.data(), 0, result.bytes());
    return result;
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_;
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_;
  }

private:
  void*  data_{};
  size_t size_{};
};

using I8Buffer  = IntBuffer<i8>;
using I16Buffer = IntBuffer<i16>;
using I32Buffer = IntBuffer<i32>;
using I64Buffer = IntBuffer<i64>;
using U8Buffer  = IntBuffer<u8>;
using U16Buffer = IntBuffer<u16>;
using U32Buffer = IntBuffer<u32>;
using U64Buffer = IntBuffer<u64>;

template<UnsignedConcept V, IndirectByteBufferConcept Backed = Buffer>
class CompressedUnsignedBuffer final : public IndirectByteBufferMixin<CompressedUnsignedBuffer<V, Backed>, V>
{
public:
  using value_type = V;

  CompressedUnsignedBuffer()  = default;
  ~CompressedUnsignedBuffer() = default;

  CompressedUnsignedBuffer(CompressedUnsignedBuffer const&) = delete;
  CompressedUnsignedBuffer(CompressedUnsignedBuffer&& rhs) noexcept
  {
    data_ = std::move(rhs.data_);
    std::swap(size_, rhs.size_);
    std::swap(element_bytes_, rhs.element_bytes_);
    std::swap(endian_, rhs.endian_);
  }

  auto operator=(CompressedUnsignedBuffer const&) -> CompressedUnsignedBuffer& = delete;
  auto operator=(CompressedUnsignedBuffer&& rhs) noexcept -> CompressedUnsignedBuffer&
  {
    if (this != &rhs) {
      data_ = std::move(rhs.data_);
      std::swap(size_, rhs.size_);
      std::swap(element_bytes_, rhs.element_bytes_);
      std::swap(endian_, rhs.endian_);
    }
    return *this;
  }

  [[nodiscard]] static auto create(size_t size, size_t element_bytes, std::endian endian = std::endian::native) -> Result<CompressedUnsignedBuffer>
    requires std::same_as<Backed, Buffer>
  {
    ASSERT_TRY(element_bytes and element_bytes <= sizeof(V), ASSUMPTION_ERROR);
    CompressedUnsignedBuffer result{};
    if (size == 0)
      return result;
    RESULT_TRY(result.data_, Buffer::create(size * element_bytes));
    result.element_bytes_ = element_bytes;
    result.size_          = size;
    result.endian_        = endian;
    return result;
  }

  [[nodiscard]] static auto create_r(char const* file, size_t size, size_t element_bytes, std::endian endian = std::endian::native) -> Result<CompressedUnsignedBuffer>
    requires std::same_as<Backed, FileBuffer>
  {
    ASSERT_TRY(element_bytes and element_bytes <= sizeof(V), ASSUMPTION_ERROR);
    CompressedUnsignedBuffer result{};
    if (size == 0)
      return result;
    RESULT_TRY(result.data_, FileBuffer::create_r(file, size * element_bytes));
    result.size_          = size;
    result.element_bytes_ = element_bytes;
    result.endian_        = endian;
    return result;
  }

  [[nodiscard]] static auto create_w(char const* file, size_t size, size_t element_bytes, bool allocate, std::endian endian = std::endian::native) -> Result<CompressedUnsignedBuffer>
    requires std::same_as<Backed, FileBuffer>
  {
    ASSERT_TRY(element_bytes and element_bytes <= sizeof(V), ASSUMPTION_ERROR);
    CompressedUnsignedBuffer result{};
    if (size == 0)
      return result;
    RESULT_TRY(result.data_, FileBuffer::create_w(file, size * element_bytes, allocate));
    result.size_          = size;
    result.element_bytes_ = element_bytes;
    result.endian_        = endian;
    return result;
  }

  [[nodiscard]] static auto create_rw(char const* file, size_t size, size_t element_bytes, bool allocate, std::endian endian = std::endian::native) -> Result<CompressedUnsignedBuffer>
    requires std::same_as<Backed, FileBuffer>
  {
    ASSERT_TRY(element_bytes and element_bytes <= sizeof(V), ASSUMPTION_ERROR);
    CompressedUnsignedBuffer result{};
    if (size == 0)
      return result;
    RESULT_TRY(result.data_, FileBuffer::create_rw(file, size * element_bytes, allocate));
    result.size_          = size;
    result.element_bytes_ = element_bytes;
    result.endian_        = endian;
    return result;
  }

  [[nodiscard]] auto get(size_t index) const -> value_type
  {
    value_type result{};
    auto       buffer_view = std::as_writable_bytes(std::span{ &result, 1 });
    auto const result_view = data_.byte_range(index * element_bytes_, element_bytes_);

    // ReSharper disable CppDFAUnreachableCode
    if constexpr (std::endian::native == std::endian::little) {
      if (endian_ == std::endian::big) {                                       // rv = 03 02 01
        std::memcpy(buffer_view.data(), result_view.data(), element_bytes_);   // bv = 03 02 01 00
        std::reverse(buffer_view.data(), buffer_view.data() + element_bytes_); // bv = 01 02 03 00
      } else {                                                                 // rv = 01 02 03
        std::memcpy(buffer_view.data(), result_view.data(), element_bytes_);   // bv = 01 02 03 00
      }
    } else {
      if (endian_ == std::endian::little) {                                        // rv = 01 02 03
        std::memcpy(buffer_view.data(), result_view.data(), element_bytes_);       // bv = 01 02 03 00
        std::reverse(buffer_view.data(), buffer_view.data() + sizeof(value_type)); // bv = 00 03 02 01
      } else {                                                                     // rv = 03 02 01
        auto const offset = sizeof(value_type) - element_bytes_;
        std::memcpy(buffer_view.data() + offset, result_view.data(), element_bytes_); // bv = 00 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode

    return result;
  }

  void set(size_t index, value_type val)
  {
    auto buffer_view = std::as_writable_bytes(std::span{ &val, 1 });
    auto result_view = data_.byte_range(index * element_bytes_, element_bytes_);

    // ReSharper disable CppDFAUnreachableCode
    if constexpr (std::endian::native == std::endian::little) { // bv = 01 02 03 00
      if (endian_ == std::endian::big) {
        std::memcpy(result_view.data(), buffer_view.data(), element_bytes_);   // rv = 01 02 03
        std::reverse(result_view.data(), result_view.data() + element_bytes_); // rv = 03 02 01
      } else {
        std::memcpy(result_view.data(), buffer_view.data(), element_bytes_); // rv = 01 02 03
      }
    } else { // bv = 00 03 02 01
      if (endian_ == std::endian::little) {
        std::reverse(buffer_view.data(), buffer_view.data() + sizeof(value_type)); // bv = 01 02 03 00
        std::memcpy(result_view.data(), buffer_view.data(), element_bytes_);       // rv = 01 02 03
      } else {
        auto const offset = sizeof(value_type) - element_bytes_;
        std::memcpy(result_view.data(), buffer_view.data() + offset, element_bytes_); // rv = 03 02 01
      }
    }
    // ReSharper restore CppDFAUnreachableCode
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_.data();
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_.data();
  }

  [[nodiscard]] auto element_bytes() const -> size_t
  {
    return element_bytes_;
  }

  [[nodiscard]] auto bytes() const -> size_t
  {
    return data_.bytes();
  }

private:
  Backed      data_{};
  size_t      size_{};
  size_t      element_bytes_{};
  std::endian endian_ = std::endian::native;
};

template<IndirectByteBufferConcept Backed = Buffer>
using CU8Buffer = CompressedUnsignedBuffer<u8, Backed>;

template<IndirectByteBufferConcept Backed = Buffer>
using CU16Buffer = CompressedUnsignedBuffer<u16, Backed>;

template<IndirectByteBufferConcept Backed = Buffer>
using CU32Buffer = CompressedUnsignedBuffer<u32, Backed>;

template<IndirectByteBufferConcept Backed = Buffer>
using CU64Buffer = CompressedUnsignedBuffer<u64, Backed>;

class BitBuffer final : public IndirectBufferMixin<BitBuffer, bool>
{
public:
  using value_type = bool;

  BitBuffer()  = default;
  ~BitBuffer() = default;

  BitBuffer(BitBuffer const&) = delete;
  BitBuffer(BitBuffer&& rhs) noexcept
  {
    data_ = std::move(rhs.data_);
    std::swap(size_, rhs.size_);
  }

  auto operator=(BitBuffer const&) -> BitBuffer& = delete;
  auto operator=(BitBuffer&& rhs) noexcept -> BitBuffer&
  {
    if (this != &rhs) {
      data_ = std::move(rhs.data_);
      std::swap(size_, rhs.size_);
    }
    return *this;
  }

  [[nodiscard]] static auto create(size_t bits) -> Result<BitBuffer>
  {
    BitBuffer result{};
    if (bits == 0)
      return result;
    auto const size = (bits + 63) / 64;
    RESULT_TRY(result.data_, IntBuffer<u64>::create(size));
    result.size_ = bits;
    return result;
  }

  [[nodiscard]] static auto zeroes(size_t size) -> Result<BitBuffer>
  {
    RESULT_TRY(auto result, create(size));
    std::memset(result.data(), 0, result.bytes());
    return result;
  }

  [[nodiscard]] auto get(size_t bit_index) const -> value_type
  {
    auto const [index, mask] = index_mask(bit_index);
    return (data_[index] & mask) != 0;
  }

  void set(size_t bit_index, value_type value)
  {
    auto const [index, mask] = index_mask(bit_index);
    auto const value_mask    = -static_cast<u64>(value);

    auto field = data_[index];  // single load
    field &= ~mask;             // unconditional unset
    field |= value_mask & mask; // conditional set
    data_[index] = field;       // single store
  }

  void set(size_t bit_index)
  {
    auto const [index, mask] = index_mask(bit_index);
    data_[index] |= mask;
  }

  void
  unset(size_t bit_index)
  {
    auto const [index, mask] = index_mask(bit_index);
    data_[index] &= ~mask;
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return size_;
  }

  [[nodiscard]] auto bytes() const -> size_t
  {
    return data_.bytes();
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_.data();
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_.data();
  }

private:
  [[nodiscard]] static constexpr auto index_mask(size_t bit_index) noexcept -> std::pair<size_t, u64>
  {
    auto const index = bit_index >> 6;
    auto const mask  = static_cast<u64>(1) << (bit_index & static_cast<size_t>(0b111111));
    return { index, mask };
  }

private:
  IntBuffer<u64> data_;
  size_t         size_ = 0;
};
