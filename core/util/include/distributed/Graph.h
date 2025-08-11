#pragma once

#include "Buffer.hpp"

namespace distributed {

template<WorldPartitionConcept Partition = TrivialSlicePartition>
struct Graph
{
  PartitionBuffer<Partition> vertices;
  Buffer                     edges;

  Graph(const char* vertices_file_name, const char* edges_file_name, std::size_t world_rank, std::size_t world_size)
  {
    // load and partition the vertex buffer from file.
    vertices = [&] {
      // file-buffer is lazy mapping the file content into memory.
      // partition buffer extracts only the partition data of this world-rank and keeps allows access with local and global indices.
      const auto vfb = FileBuffer{ vertices_file_name };
      return PartitionBuffer<Partition>{ vfb, world_rank, world_size };
    }();

    // Lambda to rebase the vertex 'begin' offset to be local to the partitioned edge buffer
    const auto element_size         = vertices.element_size >> 1;
    const auto correct_vertex_begin = [&](std::size_t index, std::size_t begin) {
      const auto vertex_begin = read_little_endian<std::size_t>(vertices[index].data(), element_size);
      write_little_endian(vertices[index].data(), vertex_begin - begin, element_size);
    };
    // Lambda to rebase the vertex 'end' offset similarly
    const auto correct_vertex_end = [&](std::size_t index, std::size_t begin) {
      const auto vertex_end = read_little_endian<std::size_t>(vertices[index].data() + element_size, element_size);
      write_little_endian(vertices[index].data() + element_size, vertex_end - begin, element_size);
    };

    // load the csr of each vertex into a local buffer and update
    // the vertex offsets to match the local ones not the global ones.
    edges = [&] {
      // file-buffer again only mapping lazy and deallocating data automatically
      const auto efb = FileBuffer{ edges_file_name };

      // in the csr data not stride other than the element size is allowed.
      // a bigger one would be waste of disc space, and overlapping makes no sense.
      assert(efb.element_stride == efb.element_size);

      // fast path: for continuous partitions and overlapping
      // vertex layout a simple memory copy can be used.
      if (Partition::continuous && vertices.element_stride < vertices.element_size && vertices.size() > 0) {

        // gather edge info over all edges
        const auto edge_begin = vertex_begin(0);
        const auto edge_end   = vertex_end(vertices.partition.size() - 1);
        const auto edge_count = edge_end - edge_begin;

        // allocate a buffer capable of holding all csr data
        Buffer buffer{ efb.element_size, efb.element_size, edge_count };

        // copy the relevant edge data from the global edge buffer
        std::memcpy(buffer.data, efb[edge_begin].data(), edge_count * efb.element_size);

        // convert all vertex offsets to point to the corresponding local edges
        for (std::size_t k = 0; k < vertices.size(); ++k)
          correct_vertex_begin(k, edge_begin);
        correct_vertex_end(vertices.size() - 1, edge_begin);

        return buffer;
      }

      // general path: for non-continuous partitions or non-overlapping layout

      std::size_t edge_count = 0;
      for (std::size_t k = 0; k < vertices.size(); ++k)
        edge_count += vertex_end(k) - vertex_begin(k);

      Buffer buffer{ efb.element_size, efb.element_size, edge_count };

      // the buffer will be filled in sequential order where
      // buffer_data and current_local_offset are our
      // iterators we use to go through the buffer
      auto*  buffer_data = buffer.data;
      std::size_t current_local_offset = 0;

      for (std::size_t k = 0; k < vertices.size(); ++k) {

        // gather the edge range info we need to copy
        const auto edge_k_begin = vertex_begin(k);
        const auto edge_k_end   = vertex_end(k);
        const auto edge_k_count = edge_k_end - edge_k_begin;

        // copy the corresponding data and update the buffer_data iterator to the end
        std::memcpy(buffer_data, efb[edge_k_begin].data(), edge_k_count * efb.element_size);
        buffer_data += edge_k_count * efb.element_size;

        // convert all vertex offsets to point to the corresponding local edges
        write_little_endian(vertices[k].data(), current_local_offset, element_size);
        current_local_offset += edge_k_count;
        write_little_endian(vertices[k].data() + element_size, current_local_offset, element_size);
      }

      return buffer;
    }();
  }

  std::size_t vertex_begin(std::size_t index) const
  {
    const auto element_size = vertices.element_size >> 1;
    return read_little_endian<std::size_t>(vertices[index].data(), element_size);
  }

  std::size_t vertex_end(std::size_t index) const
  {
    const auto element_size = vertices.element_size >> 1;
    return read_little_endian<std::size_t>(vertices[index].data() + element_size, element_size);
  }
};

}
