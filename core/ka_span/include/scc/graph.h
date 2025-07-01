#pragma once

#include "../../src/util.h"
#include <filesystem>
#include <vector>

class graph
{
public:
  std::vector<index_t> fw_beg_pos;
  std::vector<vertex_t> fw_csr;
  std::vector<index_t> bw_beg_pos;
  std::vector<vertex_t> bw_csr;

  index_t vert_count;
  index_t edge_count;

  graph() = default;
  ~graph() = default;

  graph(const std::filesystem::path& fw_beg_file,
        const std::filesystem::path& fw_csr_file,
        const std::filesystem::path& bw_beg_file,
        const std::filesystem::path& bw_csr_file);
};
