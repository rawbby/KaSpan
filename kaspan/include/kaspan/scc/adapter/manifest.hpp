#pragma once

#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/dense_unsigned_accessor.hpp>
#include <kaspan/memory/file_buffer.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/result.hpp>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace kaspan {

namespace manifest_internal {

inline auto
parse_kv_map(
  std::filesystem::path const& file) -> result<std::unordered_map<std::string,
                                                                  std::string>>
{
  std::ifstream in{ file };
  RESULT_ASSERT(in.is_open(), IO_ERROR);

  std::unordered_map<std::string, std::string> result;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '%') {
      continue;
    }

    auto const key_end = line.find(' ');
    RESULT_ASSERT(key_end != std::string::npos, DESERIALIZE_ERROR);

    // skip spaces after key to get the start of the value
    size_t value_start = key_end;
    while (value_start < line.size() && line[value_start] == ' ') {
      ++value_start;
    }

    auto const key   = line.substr(0, key_end);
    auto const value = value_start < line.size() ? line.substr(value_start) : std::string{};

    RESULT_ASSERT(result.emplace(key, value).second, DESERIALIZE_ERROR);
  }

  return result;
}

template<typename T>
  requires requires(char* it,
                    T     t) { std::from_chars(it, it, t); }
auto
parse_int(
  std::string_view value_str) -> result<T>
{
  auto const* begin = value_str.data();
  auto const* end   = begin + value_str.size();

  T t;
  auto const [ptr, ec] = std::from_chars(begin, end, t);

  // check for non empty full match
  ASSERT_EQ(ec, std::errc());
  ASSERT_EQ(ptr, end);

  return t;
}

constexpr auto
parse_bool(
  std::string_view value_str) -> result<bool>
{
  if (value_str.size() == 4) {
    ASSERT_EQ(value_str[0], 't');
    ASSERT_EQ(value_str[1], 'r');
    ASSERT_EQ(value_str[2], 'u');
    ASSERT_EQ(value_str[3], 'e');
    return true;
  }
  ASSERT_EQ(value_str.size(), 5);
  ASSERT_EQ(value_str[0], 'f');
  ASSERT_EQ(value_str[1], 'a');
  ASSERT_EQ(value_str[2], 'l');
  ASSERT_EQ(value_str[3], 's');
  ASSERT_EQ(value_str[4], 'e');
  return false;
}

constexpr auto
parse_endian(
  std::string_view value_str) -> std::endian
{
  if (value_str.size() == 6) {
    ASSERT_EQ(value_str[0], 'l');
    ASSERT_EQ(value_str[1], 'i');
    ASSERT_EQ(value_str[2], 't');
    ASSERT_EQ(value_str[3], 't');
    ASSERT_EQ(value_str[4], 'l');
    ASSERT_EQ(value_str[5], 'e');
    return std::endian::little;
  }
  ASSERT_EQ(value_str.size(), 3);
  ASSERT_EQ(value_str[0], 'b');
  ASSERT_EQ(value_str[1], 'i');
  ASSERT_EQ(value_str[2], 'g');
  return std::endian::big;
}

} // namespace manifest_internal

struct manifest
{
  u32         schema_version = 0;
  std::string graph_code;
  std::string graph_name;
  std::endian graph_endian     = std::endian::native;
  u64         graph_node_count = 0;
  u64         graph_edge_count = 0;

  bool graph_contains_self_loops      = false;
  bool graph_contains_duplicate_edges = false;

  size_t graph_head_bytes = 0;
  size_t graph_csr_bytes  = 0;

  std::filesystem::path fw_head_path;
  std::filesystem::path fw_csr_path;
  std::filesystem::path bw_head_path;
  std::filesystem::path bw_csr_path;

  static auto load(
    std::filesystem::path const& file) -> manifest
  {
    using namespace manifest_internal;

    auto const base = file.parent_path();
    ASSERT_TRY(auto const kv, parse_kv_map(file));

    auto const get_value_str = [&kv](auto const& key) -> result<std::string_view> {
      auto const it = kv.find(key);
      ASSERT(it != kv.end());
      return it->second;
    };
    auto const path_from_base = [&base](auto const& p) -> result<std::filesystem::path> {
      try {
        auto full_path = std::filesystem::canonical(base / p);
        ASSERT(std::filesystem::is_regular_file(full_path));
        return full_path;
      } catch (std::filesystem::filesystem_error const&) {
        return error_code::IO_ERROR;
      }
    };

    ASSERT_TRY(auto const schema_version_str, get_value_str("schema.version"));
    ASSERT_TRY(auto const graph_code_str, get_value_str("graph.code"));
    ASSERT_TRY(auto const graph_name_str, get_value_str("graph.name"));
    ASSERT_TRY(auto const graph_endian_str, get_value_str("graph.endian"));
    ASSERT_TRY(auto const graph_node_count_str, get_value_str("graph.node_count"));
    ASSERT_TRY(auto const graph_edge_count_str, get_value_str("graph.edge_count"));
    ASSERT_TRY(auto const graph_contains_self_loops, get_value_str("graph.contains_self_loops"));
    ASSERT_TRY(auto const graph_contains_duplicate_edges, get_value_str("graph.contains_duplicate_edges"));
    ASSERT_TRY(auto const graph_head_bytes_str, get_value_str("graph.head.bytes"));
    ASSERT_TRY(auto const graph_csr_bytes_str, get_value_str("graph.csr.bytes"));
    ASSERT_TRY(auto const fw_head_path_str, get_value_str("fw.head.path"));
    ASSERT_TRY(auto const fw_csr_path_str, get_value_str("fw.csr.path"));
    ASSERT_TRY(auto const bw_head_path_str, get_value_str("bw.head.path"));
    ASSERT_TRY(auto const bw_csr_path_str, get_value_str("bw.csr.path"));

    manifest manifest;
    ASSERT_TRY(manifest.schema_version, parse_int<u32>(schema_version_str));
    manifest.graph_code   = graph_code_str;
    manifest.graph_name   = graph_name_str;
    manifest.graph_endian = parse_endian(graph_endian_str);
    ASSERT_TRY(manifest.graph_node_count, parse_int<u64>(graph_node_count_str));
    ASSERT_TRY(manifest.graph_edge_count, parse_int<u64>(graph_edge_count_str));
    ASSERT_TRY(manifest.graph_contains_self_loops, parse_bool(graph_contains_self_loops));
    ASSERT_TRY(manifest.graph_contains_duplicate_edges, parse_bool(graph_contains_duplicate_edges));
    ASSERT_TRY(manifest.graph_head_bytes, parse_int<size_t>(graph_head_bytes_str));
    ASSERT_TRY(manifest.graph_csr_bytes, parse_int<size_t>(graph_csr_bytes_str));
    ASSERT_TRY(manifest.fw_head_path, path_from_base(fw_head_path_str));
    ASSERT_TRY(manifest.fw_csr_path, path_from_base(fw_csr_path_str));
    ASSERT_TRY(manifest.bw_head_path, path_from_base(bw_head_path_str));
    ASSERT_TRY(manifest.bw_csr_path, path_from_base(bw_csr_path_str));
    return manifest;
  }

  auto operator==(manifest const& other) const -> bool = default;
  auto operator!=(manifest const& other) const -> bool = default;
};

static auto
load_graph_from_manifest(
  manifest const& manifest) -> bidi_graph
{
  auto const  n            = manifest.graph_node_count;
  auto const  m            = manifest.graph_edge_count;
  auto const  head_bytes   = manifest.graph_head_bytes;
  auto const  csr_bytes    = manifest.graph_csr_bytes;
  auto const  load_endian  = manifest.graph_endian;
  char const* fw_head_file = manifest.fw_head_path.c_str();
  char const* fw_csr_file  = manifest.fw_csr_path.c_str();
  char const* bw_head_file = manifest.bw_head_path.c_str();
  char const* bw_csr_file  = manifest.bw_csr_path.c_str();

  ASSERT_GE(n, 0);
  ASSERT_LE(n + 1, std::numeric_limits<vertex_t>::max());

  ASSERT_GE(m, 0);
  ASSERT_LE(m, std::numeric_limits<index_t>::max());

  ASSERT_GT(head_bytes, 0);
  ASSERT_LE(head_bytes, 8);
  ASSERT_GT(csr_bytes, 0);
  ASSERT_LE(csr_bytes, 8);

  auto fw_head_buffer = file_buffer::create_r(fw_head_file, (n + 1) * head_bytes);
  auto fw_csr_buffer  = file_buffer::create_r(fw_csr_file, m * csr_bytes);
  auto bw_head_buffer = file_buffer::create_r(bw_head_file, (n + 1) * head_bytes);
  auto bw_csr_buffer  = file_buffer::create_r(bw_csr_file, m * csr_bytes);

  auto const fw_head_access = view_dense_unsigned(fw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto const fw_csr_access  = view_dense_unsigned(fw_csr_buffer.data(), m, csr_bytes, load_endian);
  auto const bw_head_access = view_dense_unsigned(bw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto const bw_csr_access  = view_dense_unsigned(bw_csr_buffer.data(), m, csr_bytes, load_endian);

  bidi_graph g(integral_cast<vertex_t>(n), integral_cast<index_t>(m));

  for (vertex_t i = 0; i < n + 1; ++i) {
    DEBUG_ASSERT_IN_RANGE(fw_head_access.get(i), 0, m + 1);
    g.fw.head[i] = integral_cast<index_t>(fw_head_access.get(i));
  }

  for (index_t i = 0; i < m; ++i) {
    DEBUG_ASSERT_IN_RANGE(fw_csr_access.get(i), 0, n);
    g.fw.csr[i] = integral_cast<vertex_t>(fw_csr_access.get(i));
  }

  for (vertex_t i = 0; i < n + 1; ++i) {
    DEBUG_ASSERT_IN_RANGE(bw_head_access.get(i), 0, m + 1);
    g.bw.head[i] = integral_cast<index_t>(bw_head_access.get(i));
  }

  for (index_t i = 0; i < m; ++i) {
    DEBUG_ASSERT_IN_RANGE(bw_csr_access.get(i), 0, n);
    g.bw.csr[i] = integral_cast<vertex_t>(bw_csr_access.get(i));
  }

  return g;
}

template<part_view_concept Part>
static auto
load_graph_part_from_manifest(
  Part     part,
  manifest const& manifest) -> bidi_graph_part<Part>
{
  DEBUG_ASSERT_EQ(manifest.graph_node_count, part.n());

  auto const  n            = part.n();
  auto const  local_n      = part.local_n();
  auto const  m            = manifest.graph_edge_count;
  auto const  head_bytes   = manifest.graph_head_bytes;
  auto const  csr_bytes    = manifest.graph_csr_bytes;
  auto const  load_endian  = manifest.graph_endian;
  auto const* fw_head_file = manifest.fw_head_path.c_str();
  auto const* fw_csr_file  = manifest.fw_csr_path.c_str();
  auto const* bw_head_file = manifest.bw_head_path.c_str();
  auto const* bw_csr_file  = manifest.bw_csr_path.c_str();

  ASSERT_LE(0, part.begin());
  ASSERT_LE(part.begin(), part.end());
  ASSERT_LE(part.end(), m);

  ASSERT_IN_RANGE(n, 0, std::numeric_limits<vertex_t>::max());

  ASSERT_GE(local_n, 0);
  ASSERT_LE(local_n, n);

  ASSERT_IN_RANGE(m, 0, std::numeric_limits<index_t>::max());

  ASSERT_GT(head_bytes, 0);
  ASSERT_LE(head_bytes, 8);
  ASSERT_GT(csr_bytes, 0);
  ASSERT_LE(csr_bytes, 8);

  auto fw_head_buffer = file_buffer::create_r(fw_head_file, (n + 1) * head_bytes);
  auto fw_csr_buffer  = file_buffer::create_r(fw_csr_file, m * csr_bytes);
  auto bw_head_buffer = file_buffer::create_r(bw_head_file, (n + 1) * head_bytes);
  auto bw_csr_buffer  = file_buffer::create_r(bw_csr_file, m * csr_bytes);

  auto fw_head_access = view_dense_unsigned(fw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto fw_csr_access  = view_dense_unsigned(fw_csr_buffer.data(), m, csr_bytes, load_endian);
  auto bw_head_access = view_dense_unsigned(bw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto bw_csr_access  = view_dense_unsigned(bw_csr_buffer.data(), m, csr_bytes, load_endian);

  auto const get_local_m = [=, &part](dense_unsigned_accessor<> const& head) -> index_t {
    if constexpr (Part::continuous()) {
      if (local_n > 0) {

        DEBUG_ASSERT_GE(head.get(part.begin()), 0);
        DEBUG_ASSERT_LE(head.get(part.begin()), m);

        DEBUG_ASSERT_GE(head.get(part.end()), 0);
        DEBUG_ASSERT_LE(head.get(part.end()), m);

        return head.get(part.end()) - head.get(part.begin());
      }
      return 0;
    } else {
      index_t sum = 0;
      for (vertex_t k = 0; k < local_n; ++k) {
        auto const u = part.select(k);

        DEBUG_ASSERT_GE(u, 0);
        DEBUG_ASSERT_LT(u, n);

        DEBUG_ASSERT_GE(head.get(u + 1), 0);
        DEBUG_ASSERT_LE(head.get(u + 1), m);

        DEBUG_ASSERT_GE(head.get(u), 0);
        DEBUG_ASSERT_LE(head.get(u), m);

        sum += head.get(u + 1) - head.get(u);
      }
      return sum;
    }
  };

  auto const local_fw_m = get_local_m(fw_head_access);
  auto const local_bw_m = get_local_m(bw_head_access);

  DEBUG_ASSERT_GE(local_fw_m, 0);
  DEBUG_ASSERT_LE(local_fw_m, m);
  DEBUG_ASSERT_GE(local_bw_m, 0);
  DEBUG_ASSERT_LE(local_bw_m, m);

  bidi_graph_part<Part> result(part, local_fw_m, local_bw_m);

  u64 pos = 0;
  for (u64 k = 0; k < local_n; ++k) {
    auto const index  = part.to_global(k);
    auto const begin  = fw_head_access.get(index);
    auto const end    = fw_head_access.get(index + 1);
    result.fw.head[k] = pos;
    for (auto it = begin; it != end; ++it) {
      result.fw.csr[pos++] = fw_csr_access.get(it);
    }
  }
  if (local_n > 0) result.fw.head[local_n] = pos;

  pos = 0;
  for (u64 k = 0; k < local_n; ++k) {
    auto const index  = part.to_global(k);
    auto const begin  = bw_head_access.get(index);
    auto const end    = bw_head_access.get(index + 1);
    result.bw.head[k] = pos;
    for (auto it = begin; it != end; ++it) {
      result.bw.csr[pos++] = bw_csr_access.get(it);
    }
  }
  if (local_n > 0) result.bw.head[local_n] = pos;

  return result;
}

} // namespace kaspan
