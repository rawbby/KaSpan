#pragma once

#include <kaspan/memory/accessor/dense_unsigned_accessor.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/adapter/stxxl_wrapper.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/util/result.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <print>
#include <string>

namespace kaspan {

namespace convert_graph_internal {

struct edge64
{
  u64 u;
  u64 v;
};

struct edge64_less
{
  constexpr auto operator()(edge64 const& lhs, edge64 const& rhs) const noexcept -> bool
  {
    return lhs.u < rhs.u or (lhs.u == rhs.u and lhs.v < rhs.v);
  }

  static constexpr auto min_value() -> edge64
  {
    return { std::numeric_limits<u64>::min(), std::numeric_limits<u64>::min() };
  }

  static constexpr auto max_value() -> edge64
  {
    return { std::numeric_limits<u64>::max(), std::numeric_limits<u64>::max() };
  }
};

struct edge64_greater
{
  constexpr auto operator()(edge64 const& lhs, edge64 const& rhs) const noexcept -> bool
  {
    return lhs.u > rhs.u or (lhs.u == rhs.u and lhs.v > rhs.v);
  }

  static constexpr auto min_value() -> edge64
  {
    return { std::numeric_limits<u64>::min(), std::numeric_limits<u64>::min() };
  }

  static constexpr auto max_value() -> edge64
  {
    return { std::numeric_limits<u64>::max(), std::numeric_limits<u64>::max() };
  }
};

inline auto
parse_uv(std::string const& s) -> edge64
{
  u64 pos = 0;

  auto skip_ws = [&] {
    while (pos < s.size() && (std::isspace(static_cast<unsigned char>(s[pos])) != 0)) {
      ++pos;
    }
  };

  auto read_u64 = [&]() -> u64 {
    u64         val      = 0;
    char const* begin    = s.data() + pos;
    char const* end      = s.data() + s.size();
    auto const [ptr, ec] = std::from_chars(begin, end, val, 10);
    ASSERT(ec == std::errc(), "failed to parse '%s'", s.c_str());
    ASSERT(ptr != begin, "failed to parse '%s'", s.c_str());
    pos = static_cast<u64>(ptr - s.data());
    return val;
  };

  skip_ws();
  auto const u = read_u64();
  skip_ws();
  auto const v = read_u64();

  // skip_ws();
  // ASSERT(pos == s.size(), "failed to parse '%s' (remaining line content)", s.c_str());

  return edge64{ u, v };
}

struct local_sort_adapter
{
  std::vector<edge64> buffer{};

  explicit local_sort_adapter(u64 memory)
  {
    buffer.reserve(memory / sizeof(edge64));
  }

  void push(edge64 edge)
  {
    buffer.push_back(edge);
  }

  void operator()()
  {
    std::ranges::sort(buffer, edge64_greater{});
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return buffer.size();
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return not buffer.empty();
  }

  auto next() -> edge64
  {
    auto const e = buffer.back();
    buffer.pop_back();
    return e;
  }

  void clear()
  {
    buffer.clear();
  }
};

struct extern_sort_adapter
{
  stxxl::sorter<edge64, edge64_less> storage;

  explicit extern_sort_adapter(u64 memory)
    : storage(edge64_less{}, memory)
  {
  }

  void push(edge64 edge)
  {
    storage.push(edge);
  }

  void operator()()
  {
    storage.sort();
  }

  [[nodiscard]] auto size() const -> size_t
  {
    return storage.size();
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return not storage.empty();
  }

  auto next() -> edge64
  {
    auto const e = *storage;
    ++storage;
    return e;
  }

  void clear()
  {
    storage.clear();
  }
};

template<typename sorter_t>
auto
convert_graph(std::string const& input_file, u64 mem_bytes, std::string const& graph_name = std::string()) -> void_result
{

  std::string const code = input_file.substr(0, input_file.find_first_of('.'));
  std::string const name = graph_name.empty() ? code : graph_name;

  std::ifstream in{ input_file };
  ASSERT(in);

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

  sorter_t sort{ mem_bytes };

  // first pass for forward
  {
    u64 max_node = 0;

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() or line[0] == '%' or line[0] == '#' or (line[0] == '/' and line[1] == '/')) {
        continue;
      }

      auto const [u, v] = parse_uv(line);
      sort.push({ u, v });

      max_node = std::max(std::max(v, u), max_node);
      has_self_loops |= u == v;
    }
    ASSERT(not in.bad());

    sort();
    m          = sort.size();
    n          = m == 0 ? 0 : max_node + 1;
    head_bytes = representing_bytes(m);
    csr_bytes  = representing_bytes(max_node);

    std::println("opening files ...");
    auto head_buffer = file_buffer::create_w<true>(fw_head_path.c_str(), (n + 1) * head_bytes);
    auto csr_buffer  = file_buffer::create_w<true>(fw_csr_path.c_str(), m * csr_bytes);
    std::println("files open");

    auto head = dense_unsigned_accessor<>::view(head_buffer.data(), head_bytes);
    auto csr  = dense_unsigned_accessor<>::view(csr_buffer.data(), csr_bytes);

    size_t i = 0;
    size_t j = 0;

    head.set(i++, 0);

    u64 current_node = 0;
    u64 current_off  = 0;

    u64 last_u = n;
    u64 last_v = n;

    while (sort.has_next()) {
      auto const [u, v] = sort.next();

      if (u == last_u and v == last_v) {
        has_duplicates = true;
      }
      last_u = u;
      last_v = v;

      while (current_node < u) {
        ++current_node;
        head.set(i++, current_off);
      }

      csr.set(j++, v);
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      head.set(i++, current_off);
    }
  }

  in.clear();
  in.seekg(0);
  DEBUG_ASSERT(not sort.has_next());
  sort.clear();

  // second pass backward
  {
    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() or line[0] == '%' or line[0] == '#' or (line[0] == '/' and line[1] == '/')) {
        continue;
      }

      auto const [u, v] = parse_uv(line);
      sort.push(edge64{ v, u });
    }

    ASSERT(not in.bad());
    sort();

    std::println("opening files ...");
    auto head_buffer = file_buffer::create_w<true>(bw_head_path.c_str(), (n + 1) * head_bytes);
    auto csr_buffer  = file_buffer::create_w<true>(bw_csr_path.c_str(), m * csr_bytes);
    std::println("files open");

    auto head = dense_unsigned_accessor<>::view(head_buffer.data(), head_bytes);
    auto csr  = dense_unsigned_accessor<>::view(csr_buffer.data(), csr_bytes);

    size_t i = 0;
    size_t j = 0;

    head.set(i++, 0);

    u64 current_node = 0;
    u64 current_off  = 0;

    while (sort.has_next()) {
      auto const [u, v] = sort.next();

      while (current_node < u) {
        ++current_node;
        head.set(i++, current_off);
      }

      csr.set(j++, v);
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      head.set(i++, current_off);
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
  return void_result::success();
}

} // namespace convert_graph_internal

inline auto
convert_graph(std::string const& input_file, u64 mem_bytes, std::string const& graph_name = std::string{}) -> void_result
{
  auto const file_size = std::filesystem::file_size(input_file);
  if (file_size < mem_bytes) {
    return convert_graph_internal::convert_graph<convert_graph_internal::local_sort_adapter>(input_file, file_size, graph_name);
  }
  return convert_graph_internal::convert_graph<convert_graph_internal::extern_sort_adapter>(input_file, file_size, graph_name);
}

} // namespace kaspan
