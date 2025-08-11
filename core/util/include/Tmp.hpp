#pragma once

#include <cctype>
#include <charconv>
#include <fstream>
#include <limits>
#include <string>
#include <stxxl/mng>
#include <stxxl/sorter>

#include "ErrorCode.hpp"
#include "Util.hpp"

template<typename T>
struct Edge
{
  using node_type = T;
  node_type src{};
  node_type dst{};
};

template<typename E>
struct EdgeLess
{
  using edge_type = E;
  using node_type = typename E::node_type;

  bool operator()(const edge_type& lhs, const edge_type& rhs) const noexcept
  {
    return lhs.src < rhs.src || (lhs.src == rhs.src && lhs.dst < rhs.dst);
  }

  static constexpr edge_type min_value()
  {
    constexpr auto lo = std::numeric_limits<node_type>::min();
    return edge_type{ lo, lo };
  }

  static constexpr edge_type max_value()
  {
    constexpr auto hi = std::numeric_limits<node_type>::max();
    return edge_type{ hi, hi };
  }
};

struct WriteStats
{
  bool has_self_loops{ false };
  bool has_duplicates{ false };
};

inline bool
is_comment_or_blank(const std::string& s)
{
  return s.empty() or s[0] == '%';
}

inline Result<std::pair<std::uint64_t, std::uint64_t>>
parse_uv(const std::string& s)
{
  std::size_t pos = 0;

  auto skip_ws = [&] {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])))
      ++pos;
  };

  auto read_u64 = [&]() -> Result<std::uint64_t> {
    std::uint64_t val    = 0;
    const char*   begin  = s.data() + pos;
    const char*   end    = s.data() + s.size();
    const auto [ptr, ec] = std::from_chars(begin, end, val, 10);
    ASSERT_TRY(ec == std::errc(), DESERIALIZE_ERROR);
    ASSERT_TRY(ptr != begin, DESERIALIZE_ERROR);
    pos = static_cast<std::size_t>(ptr - s.data());
    return val;
  };

  skip_ws();
  RESULT_TRY(const auto u, read_u64());
  skip_ws();
  RESULT_TRY(const auto v, read_u64());
  skip_ws();
  ASSERT_TRY(pos == s.size(), DESERIALIZE_ERROR);

  return std::pair{ u, v };
}

inline VoidResult
convert_graph(const std::string& input_file,
              std::uint64_t      mem_bytes,
              const std::string& graph_name = std::string())
{
  const std::string code = input_file.substr(0, input_file.find_first_of('.'));
  const std::string name = graph_name.empty() ? code : graph_name;

  std::ifstream in{ input_file };
  ASSERT_TRY(in, IO_ERROR);

  using node_t   = std::uint64_t;
  using edge_t   = Edge<node_t>;
  using cmp_t    = EdgeLess<edge_t>;
  using sorter_t = stxxl::sorter<edge_t, cmp_t>;

  std::uint64_t max_node       = 0;
  std::uint64_t m              = 0;
  std::uint64_t n              = 0;
  std::uint8_t  head_bytes     = 0;
  std::uint8_t  csr_bytes      = 0;
  bool          has_self_loops = false;
  bool          has_duplicates = false;

  const std::string manifest_path = code + ".manifest";
  const std::string fw_head_path  = code + ".fw.head.bin";
  const std::string fw_csr_path   = code + ".fw.csr.bin";
  const std::string bw_head_path  = code + ".bw.head.bin";
  const std::string bw_csr_path   = code + ".bw.csr.bin";

  // first pass for forward
  {
    sorter_t fw{ cmp_t{}, mem_bytes };

    std::string line;
    while (std::getline(in, line)) {
      if (is_comment_or_blank(line))
        continue;

      RESULT_TRY(const auto uv, parse_uv(line));
      const auto [u, v] = uv;

      fw.push(edge_t{ u, v });
      ++m;

      if (u > max_node)
        max_node = u;
      if (v > max_node)
        max_node = v;
      if (u == v)
        has_self_loops = true;
    }

    n          = m == 0 ? 0 : max_node + 1;
    head_bytes = needed_bytes(m);
    csr_bytes  = needed_bytes(max_node);

    ASSERT_TRY(not in.bad(), IO_ERROR);

    fw.sort();

    std::ofstream head_file{ fw_head_path, std::ios::binary };
    ASSERT_TRY(head_file, IO_ERROR);
    std::ofstream csr_file{ fw_csr_path, std::ios::binary };
    ASSERT_TRY(csr_file, IO_ERROR);

    RESULT_TRY(write_little_endian(head_file, static_cast<std::uint64_t>(0), head_bytes));

    std::uint64_t current_node = 0;
    std::uint64_t current_off  = 0;

    std::uint64_t last_s = n;
    std::uint64_t last_d = n;

    for (; !fw.empty(); ++fw) {
      const auto [s, d] = *fw;

      if (s == last_s and d == last_d)
        has_duplicates = true;
      last_s = s;
      last_d = d;

      while (current_node < s) {
        ++current_node;
        RESULT_TRY(write_little_endian(head_file, current_off, head_bytes));
      }

      RESULT_TRY(write_little_endian(csr_file, d, csr_bytes));
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      RESULT_TRY(write_little_endian(head_file, current_off, head_bytes));
    }

    head_file.flush();
    ASSERT_TRY(head_file, IO_ERROR);
    csr_file.flush();
    ASSERT_TRY(csr_file, IO_ERROR);
  }

  // second pass backward
  {
    in.seekg(0);
    sorter_t bw{ cmp_t{}, mem_bytes };

    std::string line;
    while (std::getline(in, line)) {
      if (is_comment_or_blank(line))
        continue;

      RESULT_TRY(const auto uv, parse_uv(line));
      const auto [u, v] = uv;

      bw.push(edge_t{ v, u });
    }

    ASSERT_TRY(not in.bad(), IO_ERROR);

    bw.sort();

    std::ofstream head_file{ bw_head_path, std::ios::binary };
    ASSERT_TRY(head_file, IO_ERROR);
    std::ofstream csr_file{ bw_csr_path, std::ios::binary };
    ASSERT_TRY(csr_file, IO_ERROR);

    RESULT_TRY(write_little_endian(head_file, static_cast<std::uint64_t>(0), head_bytes));

    std::uint64_t current_node = 0;
    std::uint64_t current_off  = 0;

    for (; !bw.empty(); ++bw) {
      const auto [s, d] = *bw;

      while (current_node < s) {
        ++current_node;
        RESULT_TRY(write_little_endian(head_file, current_off, head_bytes));
      }

      RESULT_TRY(write_little_endian(csr_file, d, csr_bytes));
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      RESULT_TRY(write_little_endian(head_file, current_off, head_bytes));
    }

    head_file.flush();
    ASSERT_TRY(head_file, IO_ERROR);
    csr_file.flush();
    ASSERT_TRY(csr_file, IO_ERROR);
  }

  std::ofstream manifest_file{ manifest_path };
  ASSERT_TRY(manifest_file, IO_ERROR);

  manifest_file << "schema.version 1\n";
  manifest_file << "graph.code " << code << "\n";
  manifest_file << "graph.name " << name << "\n";
  manifest_file << "graph.node_count " << n << "\n";
  manifest_file << "graph.edge_count " << m << "\n";
  manifest_file << "graph.contains_self_loops " << (has_self_loops ? "true" : "false") << "\n";
  manifest_file << "graph.contains_duplicate_edges " << (has_duplicates ? "true" : "false") << "\n";
  manifest_file << "fw.head.path " << fw_head_path << "\n";
  manifest_file << "fw.head.bytes " << static_cast<unsigned>(head_bytes) << "\n";
  manifest_file << "fw.csr.path " << fw_csr_path << "\n";
  manifest_file << "fw.csr.bytes " << static_cast<unsigned>(csr_bytes) << "\n";
  manifest_file << "bw.head.path " << bw_head_path << "\n";
  manifest_file << "bw.head.bytes " << static_cast<unsigned>(head_bytes) << "\n";
  manifest_file << "bw.csr.path " << bw_csr_path << "\n";
  manifest_file << "bw.csr.bytes " << static_cast<unsigned>(csr_bytes) << "\n";

  manifest_file.flush();
  ASSERT_TRY(manifest_file, IO_ERROR);
  return VoidResult::success();
}
