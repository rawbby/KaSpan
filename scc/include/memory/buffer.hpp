#pragma once

#include <util/result.hpp>
#include <util/scope_guard.hpp>

#include <bit>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

inline auto
page_size() -> u64
{
  static auto const ps = []() -> u64 {
    auto const sys_ps = sysconf(_SC_PAGESIZE);
    if (sys_ps > 0) {
      return sys_ps;
    }
    // reasonable default assumption
    return 4 * 1024; // 4 KiB;
  }();
  return ps;
}

template<typename T = byte>
auto
page_ceil(u64 size) -> u64
{
  auto const ps = page_size();
  return (size * sizeof(T) + ps - 1) / ps * ps;
}

template<typename T = byte>
auto
page_align_ceil(void const* data) -> void const*
{
  return std::bit_cast<void*>(page_ceil<T>(std::bit_cast<u64>(data)));
}

template<typename T = byte>
auto
page_align_ceil(void* data) -> void*
{
  return std::bit_cast<void*>(page_ceil<T>(std::bit_cast<u64>(data)));
}

template<typename T = byte>
auto
page_floor(u64 size) -> u64
{
  auto const ps = page_size();
  return size * sizeof(T) / ps * ps;
}

inline auto
page_align_floor(void const* data) -> void const*
{
  return std::bit_cast<void*>(page_floor(std::bit_cast<u64>(data)));
}

inline auto
page_align_floor(void* data) -> void*
{
  return std::bit_cast<void*>(page_floor(std::bit_cast<u64>(data)));
}

inline auto
is_page_aligned(void const* data) -> bool
{
  return data == page_align_floor(data);
}

namespace detail {

constexpr u8  free_count  = 4;
constexpr u8  class_count = 3;
constexpr u64 class_sizes[]{
  64 * 1024,       // 64 KiB
  2 * 1024 * 1024, // 2 MiB
  32 * 1024 * 1024 // 32 MiB
};

static void* g_free_buffer[class_count][free_count] = {};
static u8    g_count_buffer[class_count]{ 0, 0, 0 };

inline auto
pooled_page_alloc(u64 bytes) -> void*
{
  if (bytes == 0) {
    return nullptr;
  }

  for (u8 i = 0; i < class_count; ++i) {
    if (bytes <= class_sizes[i]) {
      if (g_count_buffer[i]) {
        return g_free_buffer[i][--g_count_buffer[i]];
      }
      return std::aligned_alloc(page_size(), page_ceil(class_sizes[i]));
    }
  }

  return std::aligned_alloc(page_size(), page_ceil(bytes));
}

inline void
pooled_page_free(void* ptr, u64 bytes)
{
  if (ptr == nullptr) {
    DEBUG_ASSERT_EQ(bytes, 0);
    return;
  }

  for (u8 i = 0; i < class_count; ++i) {
    if (bytes <= class_sizes[i]) {
      if (g_count_buffer[i] < free_count) {
        g_free_buffer[i][g_count_buffer[i]++] = ptr;
        return;
      }
      break;
    }
  }

  return std::free(ptr);
}

} // namespace detail

inline auto
page_aligned_alloc(u64 bytes) -> void*
{
  return detail::pooled_page_alloc(bytes);
}

inline void
page_aligned_free(void* data, u64 bytes)
{
  return detail::pooled_page_free(data, bytes);
}

constexpr auto
needed_bytes(u64 max_val) -> u8
{
  constexpr auto one = static_cast<u64>(1);
  for (u8 bytes = 1; bytes < 8; ++bytes)
    if (max_val < one << (bytes * 8))
      return bytes;
  return 8;
}

class Buffer final
{
public:
  Buffer() = default;
  ~Buffer()
  {
    if (data_ != nullptr) {
      page_aligned_free(data_, byte_size_);
      data_      = nullptr;
      byte_size_ = 0;
    }
  }

  Buffer(void* data, size_t byte_size)
    : data_(data)
    , byte_size_(byte_size)
  {
  }

  Buffer(Buffer const&) = delete;
  Buffer(Buffer&& rhs) noexcept
  {
    std::swap(data_, rhs.data_);
    std::swap(byte_size_, rhs.byte_size_);
  }

  auto operator=(Buffer const&) -> Buffer& = delete;
  auto operator=(Buffer&& rhs) noexcept -> Buffer&
  {
    if (this != &rhs) {
      std::swap(data_, rhs.data_);
      std::swap(byte_size_, rhs.byte_size_);
    }
    return *this;
  }

  template<typename T = std::byte, std::convertible_to<u64>... Sizes>
  [[nodiscard]] static auto create(Sizes... counts) -> Buffer
  {
    u64 const count = 0 + (counts + ...);
    if (count == 0)
      return Buffer{};

    auto* data = page_aligned_alloc(count * sizeof(T));
    ASSERT(data != nullptr, "failed alloc is a fatal error!");

    return Buffer{ data, count };
  }

  [[nodiscard]] auto operator[](size_t byte_index) -> std::byte
  {
    return static_cast<std::byte*>(data_)[byte_index];
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_;
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_;
  }

  [[nodiscard]] auto byte_size() const -> size_t
  {
    return byte_size_;
  }

private:
  void*  data_      = nullptr;
  size_t byte_size_ = 0;
};

class FileBuffer final
{
public:
  FileBuffer() = default;
  ~FileBuffer()
  {
    if (data_) {
      munmap(data_, byte_size_);
      data_      = nullptr;
      byte_size_ = 0;
    }
  }

  FileBuffer(void* data, size_t size)
    : data_(data)
    , byte_size_(size)
  {
  }

  FileBuffer(FileBuffer const&) = delete;
  FileBuffer(FileBuffer&& rhs) noexcept
  {
    std::swap(data_, rhs.data_);
    std::swap(byte_size_, rhs.byte_size_);
  }

  auto operator=(FileBuffer const&) -> FileBuffer& = delete;
  auto operator=(FileBuffer&& rhs) noexcept -> FileBuffer&
  {
    if (this != &rhs) {
      std::swap(data_, rhs.data_);
      std::swap(byte_size_, rhs.byte_size_);
    }
    return *this;
  }

  [[nodiscard]] static auto create_r(char const* file, size_t byte_size) -> FileBuffer
  {
    if (byte_size == 0)
      return FileBuffer{};

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

  [[nodiscard]] auto operator[](size_t byte_index) -> std::byte*
  {
    return static_cast<std::byte*>(data_) + byte_index;
  }

  [[nodiscard]] auto data() -> void*
  {
    return data_;
  }

  [[nodiscard]] auto data() const -> void const*
  {
    return data_;
  }

  [[nodiscard]] auto byte_size() const -> size_t
  {
    return byte_size_;
  }

private:
  void*  data_      = nullptr;
  size_t byte_size_ = 0;
};
