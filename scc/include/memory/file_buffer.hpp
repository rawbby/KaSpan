#pragma once

#include <util/scope_guard.hpp>

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class file_buffer final
{
public:
  file_buffer() noexcept = default;
  ~file_buffer()
  {
    if (data_ != nullptr) {
      munmap(data_, size_);
      data_ = nullptr;
      size_ = 0;
    }
  }

private:
  file_buffer(void* data, size_t size)
    : data_(data)
    , size_(size)
  {
  }

public:
  file_buffer(file_buffer const&) = delete;
  file_buffer(file_buffer&& rhs) noexcept : data_(rhs.data_), size_(rhs.size_)
  {
    
    
    rhs.data_ = nullptr;
    rhs.size_ = 0;
  }

  auto operator=(file_buffer const&) -> file_buffer& = delete;
  auto operator=(file_buffer&& rhs) noexcept -> file_buffer&
  {
    if (this != &rhs) {
      if (data_ != nullptr) { munmap(data_, size_); }
      data_     = rhs.data_;
      size_     = rhs.size_;
      rhs.data_ = nullptr;
      rhs.size_ = 0;
    }
    return *this;
  }

  [[nodiscard]] static auto create_r(char const* file, size_t byte_size) -> file_buffer
  {
    if (byte_size == 0) { return file_buffer{}; }

    auto const fd = open(file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ASSERT(fd != -1);
    SCOPE_GUARD(close(fd));

    struct stat st
    {};
    ASSERT(fstat(fd, &st) == 0);
    ASSERT(S_ISREG(st.st_mode));
    ASSERT(st.st_size == byte_size);

    auto* data = mmap(nullptr, byte_size, PROT_READ, MAP_PRIVATE, fd, 0);
    ASSERT(data != MAP_FAILED);
    return file_buffer{ data, byte_size };
  }

  template<bool allocate = false>
  [[nodiscard]] static auto create_w(char const* file, size_t size) -> file_buffer
  {
    if (size == 0) { return file_buffer{};
}

    auto const fd = [file] {
      if (allocate) { return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_CREAT | O_EXCL, 0600); }
      return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    }();
    ASSERT(fd != -1);
    SCOPE_GUARD(close(fd));

    if (allocate) {
      ASSERT(ftruncate(fd, static_cast<off_t>(size)) == 0);
    } else {
      struct stat st
      {};
      ASSERT(fstat(fd, &st) == 0);
      ASSERT(S_ISREG(st.st_mode));
      ASSERT(st.st_size == size);
    }

    auto* data = mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT(data != MAP_FAILED);
    return file_buffer{ data, size };
  }

  template<bool allocate = false>
  [[nodiscard]] static auto create_rw(char const* file, size_t byte_size) -> file_buffer
  {
    if (byte_size == 0) { return file_buffer{}; }

    auto const fd = [file] {
      if (allocate) { return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_CREAT | O_EXCL, 0600); }
      return open(file, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    }();
    ASSERT(fd != -1);
    SCOPE_GUARD(close(fd));

    if (allocate) {
      ASSERT(ftruncate(fd, static_cast<off_t>(byte_size)) == 0);
    } else {
      struct stat st
      {};
      ASSERT(fstat(fd, &st) == 0);
      ASSERT(S_ISREG(st.st_mode));
      ASSERT(st.st_size == byte_size);
    }

    auto* data = mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT(data != MAP_FAILED);
    return file_buffer{ data, byte_size };
  }

  [[nodiscard]] auto data() const -> void* { return data_; }

private:
  void*  data_ = nullptr;
  size_t size_ = 0;
};

[[nodiscard]] static auto
make_file_buffer_r(char const* file, size_t size) noexcept(false) -> file_buffer
{
  return file_buffer::create_r(file, size);
}

template<bool allocate = false>
[[nodiscard]] static auto
make_file_buffer_w(char const* file, size_t size) noexcept(false) -> file_buffer
{
  return file_buffer::create_w<allocate>(file, size);
}

template<bool allocate = false>
[[nodiscard]] static auto
make_file_buffer_rw(char const* file, size_t size) noexcept(false) -> file_buffer
{
  return file_buffer::create_rw<allocate>(file, size);
}
