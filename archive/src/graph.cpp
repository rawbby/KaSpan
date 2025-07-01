#include "../../src/graph.h"
#include "../../src/wtime.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>
#include <vector>

template<class T>
static void
read_binary(const std::filesystem::path& file, std::vector<T>& dst)
{
  std::ifstream in(file, std::ios::binary);
  if (!in)
    throw std::system_error(errno, std::generic_category(), "open " + file.string());
  in.read(reinterpret_cast<char*>(dst.data()), static_cast<std::streamsize>(dst.size() * sizeof(T)));
  if (!in)
    throw std::runtime_error("read " + file.string());
}

graph::graph(const std::filesystem::path& fw_beg_file,
             const std::filesystem::path& fw_csr_file,
             const std::filesystem::path& bw_beg_file,
             const std::filesystem::path& bw_csr_file)
{
  const double time_beg = wtime();

  vert_count = static_cast<index_t>(std::filesystem::file_size(fw_beg_file) / sizeof(index_t) - 1);
  edge_count = static_cast<index_t>(std::filesystem::file_size(fw_csr_file) / sizeof(vertex_t));

  fw_beg_pos.resize(vert_count + 1);
  fw_csr.resize(edge_count);
  bw_beg_pos.resize(vert_count + 1);
  bw_csr.resize(edge_count);

  read_binary(fw_beg_file, fw_beg_pos);
  read_binary(fw_csr_file, fw_csr);
  read_binary(bw_beg_file, bw_beg_pos);
  read_binary(bw_csr_file, bw_csr);

  const double time_end = wtime();
  const double time = time_end - time_beg;
  std::cout << "Graph load (success): " << vert_count << " Vertices, " << edge_count << " Edges (" << time << " s)\n";
}
