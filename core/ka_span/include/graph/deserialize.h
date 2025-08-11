#pragma once

#include "../../../util/include/ErrorCode.hpp"
#include "../scope_guard.h"
#include "./distributed_graph.h"

#include <bits/stl_algo.h>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ka_span {

namespace detail {

inline Result<std::uint64_t>
file_read_index(int file_descriptor, std::uint8_t bytes, std::uint64_t offset)
{
  static_assert(std::endian::native == std::endian::little);
  std::uint64_t value;
  ASSERT_TRY(pread64(file_descriptor, &value, bytes, offset) == bytes, IO_ERROR);
  return value;
}

}

VoidResult
load_graph(auto&& fn, std::uint64_t world_rank, std::uint64_t world_size, const char* file)
{
  static_assert(std::endian::native == std::endian::little);

  const auto file_descriptor = open(file, O_RDONLY);
  ASSERT_TRY(file_descriptor != -1, FILESYSTEM_ERROR);
  SCOPE_GUARD(::close(file_descriptor));

  RESULT_TRY(const auto num_global_vertices, detail::file_read_index(file_descriptor, 8, 0));
  RESULT_TRY(const auto num_global_edges, detail::file_read_index(file_descriptor, 8, 8));
  const auto num_global_offsets = num_global_vertices + 1;
  ASSERT_TRY(num_global_vertices >= world_size, ASSUMPTION_ERROR);

  const auto global_vertex_size = index_size_from_num(num_global_vertices);
  const auto global_index_size  = index_size_from_num(num_global_edges);

  const auto num_vertices_per_rank = num_global_vertices / world_size + (num_global_vertices % world_size != 0);
  const auto begin_local_offsets   = num_vertices_per_rank * world_rank;
  const auto num_local_vertices    = world_rank == world_size - 1 ? num_global_vertices - begin_local_offsets : num_vertices_per_rank;
  const auto num_local_offsets     = num_local_vertices + 1;
  const auto end_local_offsets     = begin_local_offsets + num_local_vertices;

  const auto begin_offset_memory = static_cast<std::uint64_t>(16);
  const auto begin_edge_memory   = begin_offset_memory + num_global_offsets * global_index_size;

  RESULT_TRY(const auto begin_local_edges, detail::file_read_index(file_descriptor, global_index_size, begin_offset_memory + begin_local_offsets * global_index_size));
  RESULT_TRY(const auto end_local_edges, detail::file_read_index(file_descriptor, global_index_size, begin_offset_memory + end_local_offsets * global_index_size));
  const auto num_local_edges = end_local_edges - begin_local_edges;

  const auto local_vertex_size = global_vertex_size;
  const auto local_index_size  = index_size_from_num(num_local_edges);

  return dispatch_indices_from_nums(
    [&]<typename global_vertex_type, typename global_index_type, typename local_index_type>(global_vertex_type, global_index_type, local_index_type) -> VoidResult {
      const auto page_size                = static_cast<std::uint64_t>(sysconf(_SC_PAGE_SIZE));
      const auto local_offset_buffer_size = (num_local_offsets * local_index_size + page_size - 1) / page_size * page_size;
      const auto local_edge_buffer_size   = (num_local_edges * local_vertex_size + page_size - 1) / page_size * page_size;
      const auto local_index_offset       = begin_local_edges;

      const auto local_offset_buffer = static_cast<local_index_type*>(std::aligned_alloc(page_size, local_offset_buffer_size));
      ASSERT_TRY(local_offset_buffer, ALLOCATION_ERROR, "failed to allocate offset buffer");
      SCOPE_GUARD(std::free(local_offset_buffer));

      const auto local_edge_buffer= static_cast<global_vertex_type*>(std::aligned_alloc(page_size, local_edge_buffer_size));
      ASSERT_TRY(local_edge_buffer, ALLOCATION_ERROR, "failed to allocate edge buffer");
      SCOPE_GUARD(std::free(local_edge_buffer));

      // Fill local offset buffer
      {
        const auto offset = begin_offset_memory + begin_local_offsets * global_index_size;
        const auto length = num_local_offsets * global_index_size;

        const auto map_difference = offset % page_size;
        const auto map_offset     = offset - map_difference;
        const auto map_length     = length + map_difference;

        posix_fadvise64(file_descriptor, static_cast<std::int64_t>(map_offset), static_cast<std::int64_t>(map_length), POSIX_FADV_SEQUENTIAL);
        const auto file_mapped_memory = mmap64(
          nullptr,
          map_length,
          PROT_READ,
          MAP_PRIVATE,
          file_descriptor,
          static_cast<std::int64_t>(map_offset));

        ASSERT_TRY(file_mapped_memory != MAP_FAILED, MEMORY_MAPPING_ERROR);
        madvise(file_mapped_memory, map_length, MADV_SEQUENTIAL);

        const auto raw_global_offset_buffer = static_cast<std::byte*>(file_mapped_memory) + map_difference;

        if constexpr (sizeof(local_index_type) == sizeof(global_index_type)) {
          memcpy(local_offset_buffer, raw_global_offset_buffer, num_local_offsets * global_index_size);
        } else {
          for (std::uint64_t i = 0; i < num_local_offsets; ++i) {
            global_index_type value;
            memcpy(&value, raw_global_offset_buffer + i * global_index_size, global_index_size);
            local_offset_buffer[i] = static_cast<local_index_type>(value - local_index_offset);
          }
        }

        ASSERT_TRY(!munmap(file_mapped_memory, map_length), MEMORY_MAPPING_ERROR);
        posix_fadvise64(file_descriptor, static_cast<std::int64_t>(map_offset), static_cast<std::int64_t>(map_length), POSIX_FADV_DONTNEED);
      }

      // Fill local edge buffer
      {
        const auto offset = begin_edge_memory + begin_local_edges * global_vertex_size;
        const auto length = num_local_edges * global_vertex_size;

        const auto map_difference = offset % page_size;
        const auto map_offset     = offset - map_difference;
        const auto map_length     = length + map_difference;

        posix_fadvise64(file_descriptor, static_cast<std::int64_t>(map_offset), static_cast<std::int64_t>(map_length), POSIX_FADV_SEQUENTIAL);
        const auto file_mapped_memory = mmap64(
          nullptr,
          map_length,
          PROT_READ,
          MAP_PRIVATE,
          file_descriptor,
          static_cast<std::int64_t>(map_offset));

        ASSERT_TRY(file_mapped_memory != MAP_FAILED, MEMORY_MAPPING_ERROR);
        madvise(file_mapped_memory, map_length, MADV_SEQUENTIAL);

        const auto raw_global_edge_buffer = static_cast<std::byte*>(file_mapped_memory) + map_difference;
        memcpy(local_edge_buffer, raw_global_edge_buffer, num_local_edges * global_vertex_size);
        ASSERT_TRY(!munmap(file_mapped_memory, map_length), MEMORY_MAPPING_ERROR);
        posix_fadvise64(file_descriptor, static_cast<std::int64_t>(map_offset), static_cast<std::int64_t>(map_length), POSIX_FADV_DONTNEED);
      }

      // pack and pass
      return fn(distributed_graph<global_vertex_type, global_index_type, local_index_type>{
        .num_global_vertices = static_cast<global_vertex_type>(num_global_vertices),
        .num_global_edges    = static_cast<global_index_type>(num_global_edges),
        .num_local_vertices  = static_cast<global_vertex_type>(num_local_vertices),
        .num_local_edges     = static_cast<local_index_type>(num_local_edges),
        .skipped_vertices    = static_cast<global_vertex_type>(local_index_offset),
        .csr_offset_buffer   = local_offset_buffer,
        .csr_edge_buffer     = local_edge_buffer,
      });
    },
    num_global_vertices,
    num_global_edges,
    num_local_edges);
}

}
