#pragma once

#include <util/Result.hpp>

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace manifest_internal {

inline Result<std::unordered_map<std::string, std::string>>
parse_kv_map(std::filesystem::path const& file)
{
  std::ifstream in{ file };
  RESULT_ASSERT(in.is_open(), IO_ERROR);

  std::unordered_map<std::string, std::string> result;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '%')
      continue;

    auto const key_end = line.find(' ');
    RESULT_ASSERT(key_end != std::string::npos, DESERIALIZE_ERROR);

    // skip spaces after key to get the start of the value
    size_t value_start = key_end;
    while (value_start < line.size() && line[value_start] == ' ')
      ++value_start;

    auto const key   = line.substr(0, key_end);
    auto const value = value_start < line.size() ? line.substr(value_start) : std::string{};

    RESULT_ASSERT(result.emplace(key, value).second, DESERIALIZE_ERROR);
  }

  return result;
}

template<typename T>
  requires requires(char* it, T t) { std::from_chars(it, it, t); }
Result<T>
parse_int(std::string_view value_str)
{
  auto const begin = value_str.data();
  auto const end   = begin + value_str.size();

  T t;
  auto const [ptr, ec] = std::from_chars(begin, end, t);

  // check for non empty full match
  RESULT_ASSERT(ec == std::errc(), DESERIALIZE_ERROR);
  RESULT_ASSERT(ptr == end, DESERIALIZE_ERROR);

  return t;
}

constexpr Result<bool>
parse_bool(std::string_view value_str)
{
  if (value_str.size() == 4) {
    RESULT_ASSERT(value_str[0] == 't', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[1] == 'r', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[2] == 'u', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[3] == 'e', DESERIALIZE_ERROR);
    return true;
  }
  RESULT_ASSERT(value_str.size() == 5, DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[0] == 'f', DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[1] == 'a', DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[2] == 'l', DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[3] == 's', DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[4] == 'e', DESERIALIZE_ERROR);
  return false;
}

constexpr Result<std::endian>
parse_endian(std::string_view value_str)
{
  if (value_str.size() == 6) {
    RESULT_ASSERT(value_str[0] == 'l', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[1] == 'i', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[2] == 't', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[3] == 't', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[4] == 'l', DESERIALIZE_ERROR);
    RESULT_ASSERT(value_str[5] == 'e', DESERIALIZE_ERROR);
    return std::endian::little;
  }
  RESULT_ASSERT(value_str.size() == 3, DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[0] == 'b', DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[1] == 'i', DESERIALIZE_ERROR);
  RESULT_ASSERT(value_str[2] == 'g', DESERIALIZE_ERROR);
  return std::endian::big;
}

} // namespace manifest_internal

struct Manifest
{
  u32         schema_version{};
  std::string graph_code;
  std::string graph_name;
  std::endian graph_endian;
  u64         graph_node_count{};
  u64         graph_edge_count{};

  bool graph_contains_self_loops{};
  bool graph_contains_duplicate_edges{};

  size_t graph_head_bytes{};
  size_t graph_csr_bytes{};

  std::filesystem::path fw_head_path;
  std::filesystem::path fw_csr_path;
  std::filesystem::path bw_head_path;
  std::filesystem::path bw_csr_path;

  static Result<Manifest>
  load(std::filesystem::path const& file)
  {
    using namespace manifest_internal;

    auto const base = file.parent_path();
    RESULT_TRY(auto const kv, parse_kv_map(file));

    auto const get_value_str = [&kv](auto const& key) -> Result<std::string_view> {
      auto const it = kv.find(key);
      RESULT_ASSERT(it != kv.end(), DESERIALIZE_ERROR);
      return it->second;
    };
    auto const path_from_base = [&base](auto const& p) -> Result<std::filesystem::path> {
      try {
        auto const full_path = std::filesystem::canonical(base / p);
        RESULT_ASSERT(std::filesystem::is_regular_file(full_path), IO_ERROR);
        return full_path;
      } catch (std::filesystem::filesystem_error const&) {
        return ErrorCode::IO_ERROR;
      }
    };

    RESULT_TRY(auto const schema_version_str, get_value_str("schema.version"));
    RESULT_TRY(auto const graph_code_str, get_value_str("graph.code"));
    RESULT_TRY(auto const graph_name_str, get_value_str("graph.name"));
    RESULT_TRY(auto const graph_endian_str, get_value_str("graph.endian"));
    RESULT_TRY(auto const graph_node_count_str, get_value_str("graph.node_count"));
    RESULT_TRY(auto const graph_edge_count_str, get_value_str("graph.edge_count"));
    RESULT_TRY(auto const graph_contains_self_loops, get_value_str("graph.contains_self_loops"));
    RESULT_TRY(auto const graph_contains_duplicate_edges, get_value_str("graph.contains_duplicate_edges"));
    RESULT_TRY(auto const graph_head_bytes_str, get_value_str("graph.head.bytes"));
    RESULT_TRY(auto const graph_csr_bytes_str, get_value_str("graph.csr.bytes"));
    RESULT_TRY(auto const fw_head_path_str, get_value_str("fw.head.path"));
    RESULT_TRY(auto const fw_csr_path_str, get_value_str("fw.csr.path"));
    RESULT_TRY(auto const bw_head_path_str, get_value_str("bw.head.path"));
    RESULT_TRY(auto const bw_csr_path_str, get_value_str("bw.csr.path"));

    Manifest manifest;
    RESULT_TRY(manifest.schema_version, parse_int<u32>(schema_version_str));
    manifest.graph_code = graph_code_str;
    manifest.graph_name = graph_name_str;
    RESULT_TRY(manifest.graph_endian, parse_endian(graph_endian_str));
    RESULT_TRY(manifest.graph_node_count, parse_int<u64>(graph_node_count_str));
    RESULT_TRY(manifest.graph_edge_count, parse_int<u64>(graph_edge_count_str));
    RESULT_TRY(manifest.graph_contains_self_loops, parse_bool(graph_contains_self_loops));
    RESULT_TRY(manifest.graph_contains_duplicate_edges, parse_bool(graph_contains_duplicate_edges));
    RESULT_TRY(manifest.graph_head_bytes, parse_int<size_t>(graph_head_bytes_str));
    RESULT_TRY(manifest.graph_csr_bytes, parse_int<size_t>(graph_csr_bytes_str));
    RESULT_TRY(manifest.fw_head_path, path_from_base(fw_head_path_str));
    RESULT_TRY(manifest.fw_csr_path, path_from_base(fw_csr_path_str));
    RESULT_TRY(manifest.bw_head_path, path_from_base(bw_head_path_str));
    RESULT_TRY(manifest.bw_csr_path, path_from_base(bw_csr_path_str));
    return manifest;
  }

  bool operator==(Manifest const& other) const = default;
  bool operator!=(Manifest const& other) const = default;
};
