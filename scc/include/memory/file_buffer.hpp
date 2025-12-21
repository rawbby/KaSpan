#pragma once

#include <util/scope_guard.hpp>

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class FileBuffer final
{
public:
  FileBuffer() noexcept = default;
  ~FileBuffer()
  {
    if (data_) {
      munmap(data_, size_);
      data_ = nullptr;
      size_ = 0;
    }
  }

private:
  FileBuffer(void* data, size_t size)
    : data_(data)
    , size_(size)
  {
  }

public:
  FileBuffer(FileBuffer const&) = delete;
  FileBuffer(FileBuffer&& rhs) noexcept
  {
    data_     = rhs.data_;
    size_     = rhs.size_;
    rhs.data_ = nullptr;
    rhs.size_ = 0;
  }

  auto operator=(FileBuffer const&) -> FileBuffer& = delete;
  auto operator=(FileBuffer&& rhs) noexcept -> FileBuffer&
  {
    if (this != &rhs) {
      if (data_) {
        munmap(data_, size_);
      }
      data_     = rhs.data_;
      size_     = rhs.size_;
      rhs.data_ = nullptr;
      rhs.size_ = 0;
    }
    return *this;
  }

  [[nodiscard]] static auto create_r(char const* file, size_t byte_size) -> FileBuffer
  {
    if (byte_size == 0) {
      return FileBuffer{};
    }

    auto const fd = open(file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ASSERT(fd != -1);
    SCOPE_GUARD(close(fd));

    struct stat st{};
    ASSERT(fstat(fd, &st) == 0);
    ASSERT(S_ISREG(st.st_mode));
    ASSERT(st.st_size == byte_size);

    auto* data = mmap(nullptr, byte_size, PROT_READ, MAP_PRIVATE, fd, 0);
    ASSERT(data != MAP_FAILED);
    return FileBuffer{ data, byte_size };
  }

  template<bool allocate = false>
  [[nodiscard]] static auto create_w(char const* file, size_t size) -> FileBuffer
  {
    if (size == 0)
      return FileBuffer{};

    auto const fd = [file] {
      if (allocate) {
        return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_CREAT | O_EXCL, 0600);
      }
      return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    }();
    ASSERT(fd != -1);
    SCOPE_GUARD(close(fd));

    if (allocate) {
      ASSERT(ftruncate(fd, static_cast<off_t>(size)) == 0);
    } else {
      struct stat st{};
      ASSERT(fstat(fd, &st) == 0);
      ASSERT(S_ISREG(st.st_mode));
      ASSERT(st.st_size == size);
    }

    auto* data = mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT(data != MAP_FAILED);
    return FileBuffer{ data, size };
  }

  template<bool allocate = false>
  [[nodiscard]] static auto create_rw(char const* file, size_t byte_size) -> FileBuffer
  {
    if (byte_size == 0) {
      return FileBuffer{};
    }

    auto const fd = [file] {
      if (allocate) {
        return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_CREAT | O_EXCL, 0600);
      }
      return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    }();
    ASSERT(fd != -1);
    SCOPE_GUARD(close(fd));

    if (allocate) {
      ASSERT(ftruncate(fd, static_cast<off_t>(byte_size)) == 0);
    } else {
      struct stat st{};
      ASSERT(fstat(fd, &st) == 0);
      ASSERT(S_ISREG(st.st_mode));
      ASSERT(st.st_size == byte_size);
    }

    auto* data = mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT(data != MAP_FAILED);
    return FileBuffer{ data, byte_size };
  }

  [[nodiscard]] auto data() const -> void*
  {
    return data_;
  }

private:
  void*  data_ = nullptr;
  size_t size_ = 0;
};

[[nodiscard]] static auto
make_file_buffer_r(char const* file, size_t size) noexcept(false) -> FileBuffer
{
  return FileBuffer::create_r(file, size);
}

template<bool allocate = false>
[[nodiscard]] static auto
make_file_buffer_w(char const* file, size_t size) noexcept(false) -> FileBuffer
{
  return FileBuffer::create_w<allocate>(file, size);
}

template<bool allocate = false>
[[nodiscard]] static auto
make_file_buffer_rw(char const* file, size_t size) noexcept(false) -> FileBuffer
{
  return FileBuffer::create_rw<allocate>(file, size);
}
