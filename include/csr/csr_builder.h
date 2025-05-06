#pragma once

#include <algorithm> // sort max
#include <cstdint>   // uint64_t
#include <cstdio>    // FILE
#include <utility>   // pair
#include <vector>    // vector

#include "./util.h"

namespace csr {

/// Helper structure to serialise
/// a graph into our own csr format.
template<typename index_t = std::uint64_t>
struct builder
{
  // keep track of the maximum index
  index_t num_nodes = 0;
  index_t num_edges = 0;

  // store the edges
  std::vector<std::pair<index_t, index_t>> edges;

  void push(index_t from, index_t to)
  {
    // unordered_multimap: avg O(1), worst O(n)
    // change data structure when performance problems appear
    num_nodes = std::max({ num_nodes, from, to });
    num_edges++;
    edges.emplace_back(from, to);
  }

  void serialize(FILE* file)
  {
    // last entry needs extra entry for end pointer
    const index_t row_index_count = num_nodes + 1;
    const index_t col_index_count = num_edges;

    // first store the number of bytes needed to store an index:
    const auto index_bytes = util::bytes_fast(num_nodes);  // one of 0..8
    const auto offset_bytes = util::bytes_fast(num_edges); // one of 0..8

    // write meta information to output file
    util::write(file, index_bytes, 1);
    util::write(file, offset_bytes, 1);
    util::write(file, num_nodes, index_bytes);
    util::write(file, num_edges, offset_bytes);

    // reserve temporary storage for the writing process
    std::vector<index_t> row_index;
    row_index.reserve(row_index_count);
    row_index.emplace_back(0);

    // foreach row
    std::sort(edges.begin(), edges.end());
    auto edge_it = edges.begin();

    for (index_t r = 1; r < num_nodes + 1; ++r) {

      index_t count = 0;
      while (edge_it != edges.end() && edge_it->first == r) {
        util::write(file, edge_it->second, index_bytes);
        ++count;
        ++edge_it;
      }

      row_index.emplace_back(row_index.back() + count);
    }

    util::write_n(file, row_index, offset_bytes); // write row offsets
  }
};

}
