#pragma once

#include "../Util.hpp"
#include "Partition.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <span>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace distributed {

/**
 * @brief Memory-maps a file of fixed-size elements for fast, random-access reads.
 *
 * The file must start with a 24-byte header (three little-endian uint64_t values):
 *   - element_stride: byte distance between elements
 *   - element_size:   size in bytes of each element
 *   - element_count:  number of elements in the file
 *
 * After the header, the element data is accessed directly from memory using mmap.
 *
 * - Not copyable. Moveable.
 * - All resources (file descriptor, mapping) are released on destruction or move.
 * - Throws std::runtime_error on any file or mapping error.
 * - Does not perform bounds checking on element access.
 *
 * Usage:
 *   FileBuffer buf("path/to/file");
 *   std::span<const std::byte> element = buf[42];
 *
 * Typical use case: efficient, zero-copy access to large, structured binary files.
 */
struct FileBuffer
{
  static constexpr std::size_t header_size = 3 * sizeof(std::uint64_t);

  int         fd;
  std::size_t element_stride;
  std::size_t element_size;
  std::size_t element_count;
  void*       file_map;

  explicit FileBuffer(const char* filename)
    : fd(open(filename, O_RDONLY))
    , element_stride()
    , element_size()
    , element_count()
    , file_map(nullptr)
  {
    if (fd == -1) {
      cleanup();
      throw std::runtime_error("open failed");
    }

    std::byte header[header_size]{};
    if (pread(fd, header, header_size, 0) != header_size) {
      cleanup();
      throw std::runtime_error("pread failed");
    }

    element_stride = read_little_endian<std::size_t>(header + 0 * sizeof(std::uint64_t), 8);
    element_size   = read_little_endian<std::size_t>(header + 1 * sizeof(std::uint64_t), 8);
    element_count  = read_little_endian<std::size_t>(header + 2 * sizeof(std::uint64_t), 8);

    const auto file_size = header_size + size();
    file_map             = mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_map == MAP_FAILED) {
      cleanup();
      throw std::runtime_error("mmap failed");
    }
  }

  FileBuffer(const FileBuffer&)            = delete;
  FileBuffer& operator=(const FileBuffer&) = delete;

  FileBuffer(FileBuffer&& other) noexcept
    : fd(other.fd)
    , element_stride(other.element_stride)
    , element_size(other.element_size)
    , element_count(other.element_count)
    , file_map(other.file_map)
  {
    other.fd             = -1;
    other.file_map       = nullptr;
    other.element_stride = 0;
    other.element_size   = 0;
    other.element_count  = 0;
  }

  FileBuffer& operator=(FileBuffer&& other) noexcept
  {
    if (this != &other) {
      std::swap(fd, other.fd);
      std::swap(file_map, other.file_map);
      std::swap(element_stride, other.element_stride);
      std::swap(element_size, other.element_size);
      std::swap(element_count, other.element_count);
    }
    return *this;
  }

  ~FileBuffer()
  {
    cleanup();
  }

  std::span<std::byte> operator[](std::size_t index)
  {
    const auto data = static_cast<std::byte*>(file_map) + header_size;
    return { data + index * element_stride, element_size };
  }

  std::span<const std::byte> operator[](std::size_t index) const
  {
    const auto data = static_cast<const std::byte*>(file_map) + header_size;
    return { data + index * element_stride, element_size };
  }

  std::span<const std::byte> range(std::size_t begin, std::size_t end) const
  {
    const auto data = static_cast<const std::byte*>(file_map) + header_size;
    return { data + begin * element_stride, data + (end - 1) * element_stride + element_size };
  }

  std::size_t size(std::size_t element_count) const
  {
    return (element_count - 1) * element_stride + element_size;
  }

  std::size_t size() const
  {
    return size(element_count);
  }

private:
  void cleanup()
  {
    if (file_map && file_map != MAP_FAILED) {
      const auto file_size = header_size + size();
      munmap(file_map, file_size);
      file_map = nullptr;
    }
    if (fd != -1) {
      close(fd);
      fd = -1;
    }
  }
};

/**
 * @brief Simple, move-only, aligned buffer for fixed-stride elements.
 *
 * - Allocates a single block of memory sufficient for `element_count` elements,
 *   each with the given stride and size.
 * - No partitioning or file I/O logic; user is responsible for filling and interpreting data.
 * - Ownership: buffer is freed on destruction or move.
 * - Not copyable. Moveable.
 * - No bounds checks in element access (operator[]).
 *
 * Usage:
 *   Buffer buf(stride, size, count);
 *   // Fill buf.data or copy in content
 *   auto element = buf[42]; // span over the 42nd element's data
 */
struct Buffer
{
  std::size_t element_stride;
  std::size_t element_size;
  std::size_t element_count;
  std::byte*  data;

  Buffer(std::size_t element_stride, std::size_t element_size, std::size_t element_count)
    : element_stride(element_stride)
    , element_size(element_size)
    , element_count(element_count)
    , data(aligned_alloc(element_stride * (element_count - 1) + element_size))
  {
  }

  Buffer(const Buffer&)            = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept
    : element_stride(other.element_stride)
    , element_size(other.element_size)
    , element_count(other.element_count)
    , data(other.data)
  {
    other.element_stride = 0;
    other.element_size   = 0;
    other.element_count  = 0;
    other.data           = nullptr;
  }

  Buffer& operator=(Buffer&& other) noexcept
  {
    if (this != &other) {
      std::swap(element_stride, other.element_stride);
      std::swap(element_size, other.element_size);
      std::swap(element_count, other.element_count);
      std::swap(data, other.data);
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
    return { data + index * element_stride, element_size };
  }

  std::span<const std::byte> operator[](std::size_t index) const
  {
    return { data + index * element_stride, element_size };
  }

  std::size_t size() const noexcept
  {
    return element_count;
  }
};

/**
 * @brief Owns a partitioned, aligned copy of elements from a FileBuffer according to the given Partition.
 *
 * - For continuous partitions: copies a contiguous range of elements.
 * - For arbitrary partitions: copies individually selected elements.
 * - Allocates with aligned_alloc.
 * - Not copyable. Moveable.
 * - Frees all memory on destruction.
 *
 * @tparam Partition Partitioning strategy, must provide continuous/static info, begin/end/size/select.
 *
 * Example:
 *   PartitionBuffer<MyPartition> buf(file_buffer, partition_ctor_args...);
 *   auto bytes = buf[42];
 */
template<PartitionConcept Partition>
struct PartitionBuffer
{
  Partition   partition;
  std::size_t element_stride;
  std::size_t element_size;
  std::byte*  data;

  template<typename... Args>
  explicit PartitionBuffer(const FileBuffer& fb, Args&&... args)
  {
    // todo: undocumented: partitions have the n argument always at first
    partition = Partition{ fb.element_count, std::forward<Args>(args)... };

    if constexpr (Partition::continuous) {
      element_stride = fb.element_stride;
      element_size   = fb.element_size;

      const auto range = fb.range(partition.begin, partition.end);
      data             = aligned_alloc(range.size_bytes());

      memcpy(data, range.data(), range.size_bytes());
    }

    else {
      element_stride = fb.element_size;
      element_size   = fb.element_size;
      data           = aligned_alloc(element_size * partition.size());

      for (std::size_t k = 0; k < partition.size(); ++k) {
        const auto index = partition.select(k);
        std::memcpy(data + k * element_size, fb[index], element_size);
      }
    }
  }

  PartitionBuffer(const PartitionBuffer&)            = delete;
  PartitionBuffer& operator=(const PartitionBuffer&) = delete;

  PartitionBuffer(PartitionBuffer&& other) noexcept
    : partition(other.partition)
    , element_stride(other.element_stride)
    , element_size(other.element_size)
    , data(other.data)
  {
    other.partition      = -1;
    other.element_stride = nullptr;
    other.element_size   = 0;
    other.data           = 0;
  }

  PartitionBuffer& operator=(PartitionBuffer&& other) noexcept
  {
    if (this != &other) {
      std::swap(partition, other.partition);
      std::swap(element_stride, other.element_stride);
      std::swap(element_size, other.element_size);
      std::swap(data, other.data);
    }
    return *this;
  }

  ~PartitionBuffer()
  {
    if (data) {
      std::free(data);
      data = nullptr;
    }
  }

  std::span<std::byte> operator[](std::size_t index)
  {
    return { data + index * element_stride, element_size };
  }

  std::span<const std::byte> operator[](std::size_t index) const
  {
    return { data + index * element_stride, element_size };
  }

  std::size_t size() const noexcept
  {
    return partition.size();
  }
};

}
