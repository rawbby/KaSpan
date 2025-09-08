#pragma once

#include <buffer/Accessor.hpp>
#include <util/Arithmetic.hpp>
#include <util/Util.hpp>

#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppNonExplicitConversionOperator

/// owning moveable auto freeing memory without context
template<class B>
concept BufferConcept =

  std::equality_comparable<B>

  and

  requires {
    std::is_default_constructible_v<B>;
    std::is_move_constructible_v<B>;
    std::is_move_assignable_v<B>;
    not std::is_copy_constructible_v<B>;
    not std::is_copy_assignable_v<B>;
  }

  and

  requires(B b, B const cb) {
    { b.data() } -> std::convertible_to<void*>;
    { cb.data() } -> std::convertible_to<void const*>;
  };

class Buffer final
{
public:
  Buffer() = default;
  ~Buffer()
  {
    if (data_ != nullptr) {
      std::free(data_);
      data_ = nullptr;
    }
  }

  Buffer(Buffer const&) = delete;
  Buffer(Buffer&& rhs) noexcept
  {
    std::swap(data_, rhs.data_);
  }

  auto operator=(Buffer const&) -> Buffer& = delete;
  auto operator=(Buffer&& rhs) noexcept -> Buffer&
  {
    if (this != &rhs) {
      std::swap(data_, rhs.data_);
    }
    return *this;
  }

  template<AccessorConcept Access>
  [[nodiscard]] static auto create(Access access) -> Result<Buffer>
  {
    Buffer result{};
    if (access.empty())
      return result;
    RESULT_TRY(result.data_, page_aligned_alloc(access.size_bytes()));
    return result;
  }

  template<AccessorConcept Access>
  [[nodiscard]] static auto zeroes(Access access) -> Result<Buffer>
  {
    RESULT_TRY(auto result, create(access));
    std::memset(result.data(), 0, access.size_bytes());
    return result;
  }

  // clang-format off
  [[nodiscard]] auto data() -> void* { return data_; }
  [[nodiscard]] auto data() const -> void const* { return data_; }
  operator void*() const { return data_; }
  // clang-format on

private:
  void* data_ = nullptr;
};

class FileBuffer final
{
public:
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

  template<AccessorConcept Access>
  [[nodiscard]] static auto create_r(Access access, char const* file) -> Result<FileBuffer>
  {
    FileBuffer b{};
    if (access.size_bytes() == 0)
      return b;

    b.fd_ = open(file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ASSERT_TRY(b.fd_ != -1, IO_ERROR);
    ASSERT_TRY(file_size_fd(b.fd_) == access.size_bytes(), IO_ERROR);

    b.data_ = mmap(nullptr, access.size_bytes(), PROT_READ, MAP_PRIVATE, b.fd_, 0);
    ASSERT_TRY(b.data_ != MAP_FAILED, MEMORY_MAPPING_ERROR);

    b.size_ = access.size_bytes();
    return b;
  }

  template<AccessorConcept Access>
  [[nodiscard]] static auto create_w(Access access, char const* file, bool allocate) -> Result<FileBuffer>
  {
    constexpr auto oflags       = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr auto alloc_oflags = oflags | O_CREAT | O_EXCL;

    FileBuffer b{};
    if (access.size_bytes() == 0)
      return b;

    if (allocate) {
      b.fd_ = open(file, alloc_oflags, 0600);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(ftruncate(b.fd_, static_cast<off_t>(access.size_bytes())) == 0, IO_ERROR);
    } else {
      b.fd_ = open(file, oflags);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(file_size_fd(b.fd_) == access.size_bytes(), IO_ERROR);
    }

    b.data_ = mmap(nullptr, access.size_bytes(), PROT_WRITE, MAP_SHARED, b.fd_, 0);
    ASSERT_TRY(b.data_ != MAP_FAILED, MEMORY_MAPPING_ERROR);

    b.size_ = access.size_bytes();
    return b;
  }

  template<AccessorConcept Access>
  [[nodiscard]] static auto create_rw(Access access, char const* file, bool allocate) -> Result<FileBuffer>
  {
    constexpr auto oflags       = O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr auto alloc_oflags = oflags | O_CREAT | O_EXCL;

    FileBuffer b{};
    if (access.size_bytes() == 0)
      return b;

    if (allocate) {
      b.fd_ = open(file, alloc_oflags, 0600);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(ftruncate(b.fd_, static_cast<off_t>(access.size_bytes())) == 0, IO_ERROR);
    } else {
      b.fd_ = open(file, oflags);
      ASSERT_TRY(b.fd_ != -1, IO_ERROR);
      ASSERT_TRY(file_size_fd(b.fd_) == access.size_bytes(), IO_ERROR);
    }

    b.data_ = mmap(nullptr, access.size_bytes(), PROT_READ | PROT_WRITE, MAP_SHARED, b.fd_, 0);
    ASSERT_TRY(b.data_ != MAP_FAILED, MEMORY_MAPPING_ERROR);

    b.size_ = access.size_bytes();
    return b;
  }

  // clang-format off
  [[nodiscard]] auto data() -> void* { return data_; }
  [[nodiscard]] auto data() const -> void const* { return data_; }
  operator void*() const { return data_; }
  // clang-format on

private:
  int    fd_   = -1;
  void*  data_ = nullptr;
  size_t size_ = 0;
};

// ReSharper restore CppNonExplicitConversionOperator
// ReSharper restore CppMemberFunctionMayBeStatic
