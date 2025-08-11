#pragma once

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <stxxl/mng>
#include <stxxl/sorter>

#include "Util.hpp"

template<typename T>
struct edge
{
  using node_t = T;
  node_t src, dst;
};

template<typename edge_t>
struct edge_cmp
{
  using node_t = typename edge_t::node_t;

  bool operator()(const edge_t& a, const edge_t& b) const noexcept
  {
    return a.src < b.src || (a.src == b.src && a.dst < b.dst);
  }

  static constexpr edge_t min_value()
  {
    constexpr auto min_node = std::numeric_limits<node_t>::min();
    return edge_t{ min_node, min_node };
  }

  static constexpr edge_t max_value()
  {
    constexpr auto max_node = std::numeric_limits<node_t>::max();
    return edge_t{ max_node, max_node };
  }
};

namespace distributed {

template<bool is_forward, bool write_meta>
void
convert_graph(std::istream& input, const std::string& code, std::uint64_t mem_bytes)
{
  using node_t     = std::uint64_t;
  using edge_t     = edge<node_t>;
  using edge_cmp_t = edge_cmp<edge_t>;
  using sorter_t   = stxxl::sorter<edge_t, edge_cmp_t>;
  sorter_t sorter{ edge_cmp_t{}, mem_bytes };

  node_t u, v;
  node_t node_count = 0;
  node_t edge_count = 0;
  while (input >> u >> v) {
    ++edge_count;
    node_count = std::max(node_count, std::max(u, v));

    if constexpr (is_forward)
      sorter.push({ u, v });
    else
      sorter.push({ v, u });
  }
  ++node_count;

  const auto node_bytes   = needed_bytes(node_count - 1);
  const auto offset_bytes = needed_bytes(edge_count);

  if constexpr (write_meta) {
    std::ofstream meta{ code + "_meta.bin", std::ios::binary };
    meta.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    write_little_endian(meta, node_bytes, 1);
    write_little_endian(meta, offset_bytes, 1);
    write_little_endian(meta, node_count, node_bytes);
    write_little_endian(meta, edge_count, offset_bytes);
  }

  sorter.sort();

  const auto dir_tag     = is_forward ? "_fw" : "_bw";
  auto       head_output = std::ofstream{ code + dir_tag + "_head.bin", std::ios::binary };
  auto       csr_output  = std::ofstream{ code + dir_tag + "_csr.bin", std::ios::binary };

  head_output.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  csr_output.exceptions(std::ofstream::failbit | std::ofstream::badbit);

  write_little_endian(head_output, 0, offset_bytes);
  auto current_node   = static_cast<node_t>(0);
  auto current_offset = static_cast<node_t>(0);
  for (; !sorter.empty(); ++sorter) {
    const auto& [src, dst] = *sorter;
    while (current_node < src) {
      ++current_node;
      write_little_endian(head_output, current_offset, offset_bytes);
    }
    write_little_endian(csr_output, dst, node_bytes);
    ++current_offset;
  }
  while (current_node < node_count) {
    ++current_node;
    write_little_endian(head_output, current_offset, offset_bytes);
  }
}

/**
 * This function is used to convert a text based edge list file into a binary format while not allocating much more memory than requested.
 * The binary format is in multi file structure.
 * It not only creates a compressed sparse version of the graph but also stores metadata and the backward graph.
 *
 * The result contains five files:
 * 1. **code**_meta.bin:    contains metadata like the number of nodes and edges and the bytes per value used while writing.
 * 2. **code**_fw_head.bin: per node contains the begin and end offset of the csr.
 * 3. **code**_fw_csr.bin:  contains the csr data continuous.
 * 4. **code**_bw_head.bin: per node contains the begin and end offset of the csr.
 * 5. **code**_bw_csr.bin:  contains the csr data continuous.
 *
 * notice that a sideeffect of the conversion is also that all csr data is guaranteed to be sorted.
 *
 * @param input_file A path to an existing edge list file. Notice for **code**.txt the code is used as basename for the output files.
 * @param mem_bytes The amount of memory this function is allowed to allocate.
 */
inline void
convert_graph(const std::string& input_file, std::uint64_t mem_bytes)
{
  const std::string code = input_file.substr(0, input_file.find_first_of('.'));

  std::ifstream input{ input_file };
  input.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  convert_graph<true, true>(input, code, mem_bytes);
  input.clear();
  input.seekg(0);

  convert_graph<false, false>(input, code, mem_bytes);
  input.close();
}

}
