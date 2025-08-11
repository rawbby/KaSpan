#pragma once

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ErrorCode.hpp"

struct Manifest
{
  std::uint32_t schema_version;
  std::string   graph_code;
  std::string   graph_name;
  std::uint64_t graph_node_count;
  std::uint64_t graph_edge_count;

  bool graph_contains_self_loops;
  bool graph_contains_duplicate_edges;

  std::filesystem::path fw_head_path;
  std::size_t           fw_head_bytes;
  std::filesystem::path fw_csr_path;
  std::size_t           fw_csr_bytes;

  std::filesystem::path bw_head_path;
  std::size_t           bw_head_bytes;
  std::filesystem::path bw_csr_path;
  std::size_t           bw_csr_bytes;
};

inline Result<std::unordered_map<std::string, std::string>>
parse_kv_map(const std::filesystem::path& file)
{
  std::ifstream in{ file };
  ASSERT_TRY(in.is_open(), IO_ERROR);

  std::unordered_map<std::string, std::string> result;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '%')
      continue;

    const auto key_end = line.find(' ');
    ASSERT_TRY(key_end != std::string::npos, DESERIALIZE_ERROR);

    const auto value_end = line.size();
    ASSERT_TRY(result.emplace(line.substr(0, key_end), line.substr(key_end + 1, value_end)).second, DESERIALIZE_ERROR);
  }

  return result;
}

template<typename T>
  requires requires(char* it, T t) { std::from_chars(it, it, t); }
Result<T>
parse_int(std::string_view value_str)
{
  const auto begin = value_str.data();
  const auto end   = begin + value_str.size();

  T t;
  const auto [ptr, ec] = std::from_chars(begin, end, t);

  // check for non empty full match
  ASSERT_TRY(ec == std::errc(), DESERIALIZE_ERROR);
  ASSERT_TRY(ptr == end, DESERIALIZE_ERROR);

  return t;
}

constexpr Result<bool>
parse_bool(std::string_view value_str)
{
  if (value_str.size() == 4) {
    ASSERT_TRY(value_str[0] == 't', DESERIALIZE_ERROR);
    ASSERT_TRY(value_str[1] == 'r', DESERIALIZE_ERROR);
    ASSERT_TRY(value_str[2] == 'u', DESERIALIZE_ERROR);
    ASSERT_TRY(value_str[3] == 'e', DESERIALIZE_ERROR);
    return true;
  }
  ASSERT_TRY(value_str.size() == 5, DESERIALIZE_ERROR);
  ASSERT_TRY(value_str[0] == 'f', DESERIALIZE_ERROR);
  ASSERT_TRY(value_str[1] == 'a', DESERIALIZE_ERROR);
  ASSERT_TRY(value_str[2] == 'l', DESERIALIZE_ERROR);
  ASSERT_TRY(value_str[3] == 's', DESERIALIZE_ERROR);
  ASSERT_TRY(value_str[4] == 'e', DESERIALIZE_ERROR);
  return false;
}

inline Result<Manifest>
load_graph_coordinator(const std::filesystem::path& file)
{
  const auto base = file.parent_path();
  RESULT_TRY(const auto kv, parse_kv_map(file));

  const auto get_value_str = [&kv](const auto& key) -> Result<std::string_view> {
    const auto it = kv.find(key);
    ASSERT_TRY(it != kv.end(), DESERIALIZE_ERROR);
    return it->second;
  };
  const auto path_from_base = [&base](const auto& p) -> Result<std::filesystem::path> {
    try {
      const auto full_path = std::filesystem::canonical(base / p);
      ASSERT_TRY(std::filesystem::is_regular_file(full_path), IO_ERROR);
      return full_path;
    } catch (const std::filesystem::filesystem_error&) {
      return ErrorCode::IO_ERROR;
    }
  };

  RESULT_TRY(const auto schema_version_str, get_value_str("schema.version"));
  RESULT_TRY(const auto graph_code_str, get_value_str("graph.code"));
  RESULT_TRY(const auto graph_name_str, get_value_str("graph.name"));
  RESULT_TRY(const auto graph_node_count_str, get_value_str("graph.node_count"));
  RESULT_TRY(const auto graph_edge_count_str, get_value_str("graph.edge_count"));
  RESULT_TRY(const auto graph_contains_self_loops, get_value_str("graph.contains_self_loops"));
  RESULT_TRY(const auto graph_contains_duplicate_edges, get_value_str("graph.contains_duplicate_edges"));
  RESULT_TRY(const auto fw_head_path_str, get_value_str("fw.head.path"));
  RESULT_TRY(const auto fw_head_bytes_str, get_value_str("fw.head.bytes"));
  RESULT_TRY(const auto fw_csr_path_str, get_value_str("fw.csr.path"));
  RESULT_TRY(const auto fw_csr_bytes_str, get_value_str("fw.csr.bytes"));
  RESULT_TRY(const auto bw_head_path_str, get_value_str("bw.head.path"));
  RESULT_TRY(const auto bw_head_bytes_str, get_value_str("bw.head.bytes"));
  RESULT_TRY(const auto bw_csr_path_str, get_value_str("bw.csr.path"));
  RESULT_TRY(const auto bw_csr_bytes_str, get_value_str("bw.csr.bytes"));

  Manifest manifest;
  RESULT_TRY(manifest.schema_version, parse_int<std::uint32_t>(schema_version_str));
  manifest.graph_code = graph_code_str;
  manifest.graph_name = graph_name_str;
  RESULT_TRY(manifest.graph_node_count, parse_int<std::uint64_t>(graph_node_count_str));
  RESULT_TRY(manifest.graph_edge_count, parse_int<std::uint64_t>(graph_edge_count_str));
  RESULT_TRY(manifest.graph_contains_self_loops, parse_bool(graph_contains_self_loops));
  RESULT_TRY(manifest.graph_contains_duplicate_edges, parse_bool(graph_contains_duplicate_edges));
  RESULT_TRY(manifest.fw_head_path, path_from_base(fw_head_path_str));
  RESULT_TRY(manifest.fw_head_bytes, parse_int<std::size_t>(fw_head_bytes_str));
  RESULT_TRY(manifest.fw_csr_path, path_from_base(fw_csr_path_str));
  RESULT_TRY(manifest.fw_csr_bytes, parse_int<std::size_t>(fw_csr_bytes_str));
  RESULT_TRY(manifest.bw_head_path, path_from_base(bw_head_path_str));
  RESULT_TRY(manifest.bw_head_bytes, parse_int<std::size_t>(bw_head_bytes_str));
  RESULT_TRY(manifest.bw_csr_path, path_from_base(bw_csr_path_str));
  RESULT_TRY(manifest.bw_csr_bytes, parse_int<std::size_t>(bw_csr_bytes_str));
  return manifest;
}
