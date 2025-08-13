#pragma once

#include <Util.hpp>
#include <distributed/Partition.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <span>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace distributed {

/**
 * FileBuffer
 *
 * Read-only, non-owning view over a fixed-width element array stored on disk.
 * - Maps the file with mmap (no header on disk; flat array of fixed-width elements).
 * - Element layout: a flat array of element_count elements, each element_size bytes.
 * - Not copyable. Moveable.
 * - All resources are released on destruction or move.
 *
 * Creation is done via the static create method to propagate I/O/mapping errors.
 */
struct FileBuffer
{
  int         fd;
  std::size_t element_size;
  std::size_t element_count;
  void*       file_map;

  FileBuffer() noexcept
    : fd(-1)
    , element_size()
    , element_count()
    , file_map(nullptr)
  {
  }

  static Result<FileBuffer> create(const char* filename, std::size_t element_bytes, std::size_t element_count)
  {
    FileBuffer out;
    out.element_size  = element_bytes;
    out.element_count = element_count;
    out.fd            = open(filename, O_RDONLY);
    ASSERT_TRY(out.fd != -1, IO_ERROR);

    const auto expected_bytes = element_bytes * element_count;

    struct stat st;
    ASSERT_TRY(fstat(out.fd, &st) == 0, IO_ERROR);
    ASSERT_TRY(st.st_size >= 0, IO_ERROR);
    ASSERT_TRY(static_cast<std::size_t>(st.st_size) == expected_bytes, IO_ERROR);

    out.file_map = mmap(nullptr, expected_bytes, PROT_READ, MAP_SHARED, out.fd, 0);
    ASSERT_TRY(out.file_map != MAP_FAILED, MEMORY_MAPPING_ERROR);

    return out;
  }

  FileBuffer(const FileBuffer&)            = delete;
  FileBuffer& operator=(const FileBuffer&) = delete;

  FileBuffer(FileBuffer&& other) noexcept
    : fd(other.fd)
    , element_size(other.element_size)
    , element_count(other.element_count)
    , file_map(other.file_map)
  {
    other.fd            = -1;
    other.element_size  = 0;
    other.element_count = 0;
    other.file_map      = nullptr;
  }

  FileBuffer& operator=(FileBuffer&& other) noexcept
  {
    if (this != &other) {
      std::swap(other.fd, fd);
      std::swap(other.element_size, element_size);
      std::swap(other.element_count, element_count);
      std::swap(other.file_map, file_map);
    }
    return *this;
  }

  ~FileBuffer()
  {
    if (file_map && file_map != MAP_FAILED) {
      munmap(file_map, element_size * element_count);
      file_map = nullptr;
    }
    if (fd != -1) {
      close(fd);
      fd = -1;
    }
  }

  std::span<const std::byte> operator[](std::size_t index) const
  {
    const auto base = static_cast<const std::byte*>(file_map);
    return { base + index * element_size, element_size };
  }

  std::span<const std::byte> range(std::size_t begin, std::size_t end) const
  {
    const auto base = static_cast<const std::byte*>(file_map);
    return { base + begin * element_size, (end - begin) * element_size };
  }

  template<typename T>
  T get(std::size_t index) const
  {
    const auto base = static_cast<const std::byte*>(file_map);
    return read_little_endian<T>(base + index * element_size, element_size);
  }

  std::size_t size() const noexcept
  {
    return element_count;
  }
};

/**
 * Buffer
 *
 * Owning, page-aligned, contiguous byte storage for a fixed-width element array.
 * - Element layout: a flat array of element_count elements, each element_size bytes.
 * - Not copyable. Moveable.
 * - Memory is freed on destruction or move.
 *
 * Creation is done via the static create method to propagate allocation errors.
 */
struct Buffer
{
  std::size_t element_size;
  std::size_t element_count;
  std::byte*  data;

  Buffer() noexcept
    : element_size(0)
    , element_count(0)
    , data(nullptr)
  {
  }

  static Result<Buffer> create(std::size_t element_bytes, std::size_t element_count)
  {
    Buffer buf;
    buf.element_size  = element_bytes;
    buf.element_count = element_count;
    RESULT_TRY(buf.data, page_aligned_alloc(element_bytes * element_count));
    return buf;
  }

  Buffer(const Buffer&)            = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept
    : element_size(other.element_size)
    , element_count(other.element_count)
    , data(other.data)
  {
    other.element_size  = 0;
    other.element_count = 0;
    other.data          = nullptr;
  }

  Buffer& operator=(Buffer&& other) noexcept
  {
    if (this != &other) {
      std::swap(other.element_size, element_size);
      std::swap(other.element_count, element_count);
      std::swap(other.data, data);
    }
    return *this;
  }

  ~Buffer()
  {
    if (data) {
      std::free(data);
      data = nullptr;
    }
  }

  std::span<std::byte> operator[](std::size_t index)
  {
    return { data + index * element_size, element_size };
  }

  std::span<const std::byte> operator[](std::size_t index) const
  {
    return { data + index * element_size, element_size };
  }

  std::span<std::byte> range(std::size_t begin, std::size_t end)
  {
    return { data + begin * element_size, (end - begin) * element_size };
  }

  std::span<const std::byte> range(std::size_t begin, std::size_t end) const
  {
    return { data + begin * element_size, (end - begin) * element_size };
  }

  template<typename T>
  T get(std::size_t index) const
  {
    return read_little_endian<T>(data + index * element_size, element_size);
  }

  template<typename T>
  void set(std::size_t index, T value) const
  {
    return write_little_endian<T>(data + index * element_size, value, element_size);
  }

  std::size_t size() const noexcept
  {
    return element_count;
  }
};

VoidResult
copy_buffer(const FileBuffer& fb, Buffer& out, PartitionConcept auto partition)
{
  using Partition = decltype(partition);
  ASSERT_TRY(out.element_size == fb.element_size, ERROR);
  ASSERT_TRY(out.element_count == partition.size(), ERROR);
  ASSERT_TRY(fb.element_count == partition.n, ERROR);

  if constexpr (Partition::continuous) {
    const auto range = fb.range(partition.begin, partition.end);
    const auto bytes = range.size();

    RESULT_TRY(out.data, page_aligned_alloc(bytes));
    std::memcpy(out.data, range.data(), bytes);
  } else {
    const std::size_t count = partition.size();
    const std::size_t bytes = out.element_size * count;

    RESULT_TRY(out.data, page_aligned_alloc(bytes));
    for (std::size_t k = 0; k < count; ++k) {
      const auto index = partition.select(k);
      std::memcpy(out.data + k * out.element_size, fb[index].data(), out.element_size);
    }
  }

  return VoidResult::success();
}

} // namespace distributed
