#pragma once

#include <scc/base.hpp>
#include <memory/Buffer.hpp>
#include <util/Result.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <string>
#include <stxxl/mng>
#include <stxxl/sorter>
#include <utility>

namespace convert_graph_internal {

inline auto
parse_uv(std::string const& s) -> Result<std::pair<u64, u64>>
{
  u64 pos = 0;

  auto skip_ws = [&] {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])))
      ++pos;
  };

  auto read_u64 = [&]() -> Result<u64> {
    u64         val      = 0;
    char const* begin    = s.data() + pos;
    char const* end      = s.data() + s.size();
    auto const [ptr, ec] = std::from_chars(begin, end, val, 10);
    RESULT_ASSERT(ec == std::errc(), DESERIALIZE_ERROR);
    RESULT_ASSERT(ptr != begin, DESERIALIZE_ERROR);
    pos = static_cast<u64>(ptr - s.data());
    return val;
  };

  skip_ws();
  RESULT_TRY(auto const u, read_u64());
  skip_ws();
  RESULT_TRY(auto const v, read_u64());
  skip_ws();
  RESULT_ASSERT(pos == s.size(), DESERIALIZE_ERROR);

  return std::pair{ u, v };
}

} // namespace convert_graph_internal

inline auto
convert_graph(std::string const& input_file,
              u64                mem_bytes,
              std::string const& graph_name = std::string()) -> VoidResult
{
  using namespace convert_graph_internal;

  std::string const code = input_file.substr(0, input_file.find_first_of('.'));
  std::string const name = graph_name.empty() ? code : graph_name;

  std::ifstream in{ input_file };
  RESULT_ASSERT(in, IO_ERROR);

  u64  max_node       = 0;
  u64  m              = 0;
  u64  n              = 0;
  u8   head_bytes     = 0;
  u8   csr_bytes      = 0;
  bool has_self_loops = false;
  bool has_duplicates = false;

  std::string const manifest_path = code + ".manifest";
  std::string const fw_head_path  = code + ".fw.head.bin";
  std::string const fw_csr_path   = code + ".fw.csr.bin";
  std::string const bw_head_path  = code + ".bw.head.bin";
  std::string const bw_csr_path   = code + ".bw.csr.bin";

  // first pass for forward
  {
    stxxl::sorter<Edge, decltype(edge_less)> fw{ edge_less, mem_bytes };

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() or line[0] == '%' or line[0] == '#' or (line[0] == '/' and line[1] == '/'))
        continue;

      RESULT_TRY(auto const uv, parse_uv(line));
      auto const [u, v] = uv;

      fw.push(Edge{ u, v });
      ++m;

      max_node = std::max(u, max_node);
      max_node = std::max(v, max_node);
      if (u == v)
        has_self_loops = true;
    }

    n          = m == 0 ? 0 : max_node + 1;
    head_bytes = needed_bytes(m);
    csr_bytes  = needed_bytes(max_node);

    RESULT_ASSERT(not in.bad(), IO_ERROR);

    fw.sort();

    RESULT_TRY(auto head_file, CU64Buffer<FileBuffer>::create_w(fw_head_path.c_str(), n + 1, head_bytes, true));
    RESULT_TRY(auto csr_file, CU64Buffer<FileBuffer>::create_w(fw_csr_path.c_str(), m, csr_bytes, true));

    size_t i = 0;
    size_t j = 0;

    head_file.set(i++, 0);

    u64 current_node = 0;
    u64 current_off  = 0;

    u64 last_u = n;
    u64 last_v = n;

    for (; !fw.empty(); ++fw) {
      auto const [u, v] = *fw;

      if (u == last_u and v == last_v)
        has_duplicates = true;
      last_u = u;
      last_v = v;

      while (current_node < u) {
        ++current_node;
        head_file.set(i++, current_off);
      }

      csr_file.set(j++, v);
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      head_file.set(i++, current_off);
    }
  }

  // second pass backward
  {
    in.clear();
    in.seekg(0);

    stxxl::sorter<Edge, EdgeLess> bw{ EdgeLess{}, mem_bytes };

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() or line[0] == '%' or line[0] == '#' or (line[0] == '/' and line[1] == '/'))
        continue;

      RESULT_TRY(auto const uv, parse_uv(line));
      auto const [u, v] = uv;

      bw.push(Edge{ v, u });
    }

    RESULT_ASSERT(not in.bad(), IO_ERROR);

    bw.sort();

    RESULT_TRY(auto head_file, CU64Buffer<FileBuffer>::create_w(bw_head_path.c_str(), n + 1, head_bytes, true));
    RESULT_TRY(auto csr_file, CU64Buffer<FileBuffer>::create_w(bw_csr_path.c_str(), m, csr_bytes, true));

    size_t i = 0;
    size_t j = 0;

    head_file.set(i++, 0);

    u64 current_node = 0;
    u64 current_off  = 0;

    for (; !bw.empty(); ++bw) {
      auto const [u, v] = *bw;

      while (current_node < u) {
        ++current_node;
        head_file.set(i++, current_off);
      }

      csr_file.set(j++, v);
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      head_file.set(i++, current_off);
    }
  }

  std::ofstream manifest_file{ manifest_path };
  RESULT_ASSERT(manifest_file, IO_ERROR);

  // ReSharper disable once CppDFAUnreachableCode
  constexpr auto endian_str = std::endian::native == std::endian::big ? "big" : "little";

  manifest_file << "schema.version 1\n";
  manifest_file << "graph.code " << code << '\n';
  manifest_file << "graph.name " << name << '\n';
  manifest_file << "graph.endian " << endian_str << '\n';
  manifest_file << "graph.node_count " << n << '\n';
  manifest_file << "graph.edge_count " << m << '\n';
  manifest_file << "graph.contains_self_loops " << (has_self_loops ? "true" : "false") << '\n';
  manifest_file << "graph.contains_duplicate_edges " << (has_duplicates ? "true" : "false") << '\n';
  manifest_file << "graph.head.bytes " << static_cast<unsigned>(head_bytes) << '\n';
  manifest_file << "graph.csr.bytes " << static_cast<unsigned>(csr_bytes) << '\n';
  manifest_file << "fw.head.path " << fw_head_path << '\n';
  manifest_file << "fw.csr.path " << fw_csr_path << '\n';
  manifest_file << "bw.head.path " << bw_head_path << '\n';
  manifest_file << "bw.csr.path " << bw_csr_path << '\n';

  manifest_file.flush();
  RESULT_ASSERT(manifest_file, IO_ERROR);
  return VoidResult::success();
}
