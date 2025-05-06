#pragma once

#include <bit>       // bit_ceil
#include <cstddef>   // size_t
#include <cstdint>   // uint8_t uint64_t
#include <cstdio>    // FILE fseeko off_t SEEK_SET
#include <cstdlib>   // malloc
#include <memory>    // malloc
#include <ranges>    // views::iota
#include <span>      // span
#include <stdexcept> // runtime_error

#include "./util.h"

namespace csr {

template<typename T, typename O>
struct distributed_csr
{
  using index_t = T;
  using offset_t = O;

  index_t num_local_nodes;
  offset_t num_local_edges;

  index_t skipped_nodes;
  offset_t skipped_edges;

  offset_t* node_offsets;
  index_t* edges;

  auto operator[](index_t node) { return std::span<index_t>{ edges + node_offsets[node - skipped_nodes - 1] - skipped_edges, edges + node_offsets[node - skipped_nodes] - skipped_edges }; }

  auto local_nodes() { return std::views::iota(static_cast<index_t>(skipped_nodes + 1), static_cast<index_t>(skipped_nodes + num_local_nodes + 1)); }

  distributed_csr(FILE* file, std::uint8_t index_bytes, std::uint8_t offset_bytes, std::size_t node_index, std::size_t node_count)
  {
    if (std::bit_ceil(index_bytes) != sizeof(index_t))
      throw std::runtime_error("distributed_csr initialised with mismatching index type. please use dispatch_distributed_csr!");
    if (std::bit_ceil(offset_bytes) != sizeof(offset_t))
      throw std::runtime_error("distributed_csr initialised with mismatching offset type. please use dispatch_distributed_csr!");

    const std::uint64_t header_bytes = 2 + index_bytes + offset_bytes;

    index_t num_nodes;
    offset_t num_edges;
    util::read(file, num_nodes, index_bytes);
    util::read(file, num_edges, offset_bytes);

    num_local_nodes = num_nodes / node_count;
    skipped_nodes = static_cast<index_t>(node_index) * num_local_nodes;

    // last node gets remaining nodes
    if (node_index == node_count - 1) {
      num_local_nodes += num_nodes - num_local_nodes * node_count;
    }

    /* ------------------------------------------ *\
     * Read begin and end position of local edges *
    \* ------------------------------------------ */

    const std::uint64_t adjacency_begin = header_bytes;
    const std::uint64_t adjacency_bytes = num_edges * index_bytes;
    const std::uint64_t node_offsets_beg = adjacency_begin + adjacency_bytes;
    const std::uint64_t local_node_offsets_beg = node_offsets_beg + skipped_nodes * offset_bytes;
    const std::uint64_t local_node_offsets_end = node_offsets_beg + (skipped_nodes + num_local_nodes) * offset_bytes;

    offset_t local_edges_begin, local_edges_end;

    // read end first
    if (fseeko(file, static_cast<off_t>(local_node_offsets_end), SEEK_SET) != 0)
      throw std::runtime_error("file seek to offsets unexpectedly failed");
    util::read(file, local_edges_end, offset_bytes);

    // read begin last (so we dont need to seek again when reading node offsets)
    if (fseeko(file, static_cast<off_t>(local_node_offsets_beg), SEEK_SET) != 0)
      throw std::runtime_error("file seek to offset unexpectedly failed");
    util::read(file, local_edges_begin, offset_bytes);

    skipped_edges = local_edges_begin;
    num_local_edges = local_edges_end - local_edges_begin;

    /* ------------------------------------------------- *\
     * Allocate the memory for the main data of this csr *
    \* ------------------------------------------------- */

    auto* mem = static_cast<std::byte*>(std::malloc((num_local_nodes + 1) * sizeof(offset_t) + num_local_edges * sizeof(index_t)));
    if (not mem)
      throw std::runtime_error("failed to allocate enough memory for csr data structure!");

    node_offsets = reinterpret_cast<offset_t*>(mem);
    edges = reinterpret_cast<index_t*>(mem + (num_local_nodes + 1) * sizeof(offset_t));

    /* -------------------------------------- *\
     * Fill the buffer with the right content *
    \* -------------------------------------- */

    node_offsets[0] = skipped_edges;
    util::read_n(file, std::span{ node_offsets + 1, num_local_nodes }, offset_bytes);

    const std::uint64_t local_adjacency_beg = adjacency_begin + skipped_edges * index_bytes;
    if (fseeko(file, static_cast<off_t>(local_adjacency_beg), SEEK_SET) != 0)
      throw std::runtime_error("file seek to offset unexpectedly failed");

    util::read_n(file, std::span{ edges, num_local_edges }, index_bytes);
  }

  ~distributed_csr() { std::free(node_offsets); }
};

void
load_distributed(FILE* file, std::size_t node_index, std::size_t node_count, auto&& fp)
{
  std::uint8_t index_bytes;
  std::uint8_t offset_bytes;
  util::read(file, index_bytes, 1);
  util::read(file, offset_bytes, 1);

  // clang-format off
  util::dispatch_uint(index_bytes,  [&]([[maybe_unused]] auto index_dummy ) {
    util::dispatch_uint(offset_bytes, [&]([[maybe_unused]] auto offset_dummy) {
      using index_t = decltype(index_dummy);
      using offset_t =  decltype(offset_dummy);

      fp(distributed_csr<index_t, offset_t>{
        file, index_bytes, offset_bytes, node_index, node_count
      });
    });
  });
  // clang-format on
}

}
