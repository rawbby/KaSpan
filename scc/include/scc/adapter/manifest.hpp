#pragma once

#include <scc/graph.hpp>
#include <util/result.hpp>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace manifest_internal {

inline auto
parse_kv_map(std::filesystem::path const& file) -> Result<std::unordered_map<std::string, std::string>>
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
auto
parse_int(std::string_view value_str) -> Result<T>
{
  auto const* begin = value_str.data();
  auto const* end   = begin + value_str.size();

  T t;
  auto const [ptr, ec] = std::from_chars(begin, end, t);

  // check for non empty full match
  RESULT_ASSERT(ec == std::errc(), DESERIALIZE_ERROR);
  RESULT_ASSERT(ptr == end, DESERIALIZE_ERROR);

  return t;
}

constexpr auto
parse_bool(std::string_view value_str) -> Result<bool>
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

constexpr auto
parse_endian(std::string_view value_str) -> Result<std::endian>
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

  static auto
  load(std::filesystem::path const& file) -> Result<Manifest>
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
        auto full_path = std::filesystem::canonical(base / p);
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

  auto operator==(Manifest const& other) const -> bool = default;
  auto operator!=(Manifest const& other) const -> bool = default;
};

static auto
load_graph_from_manifest(Manifest const& manifest, void* memory) -> Result<Graph>
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

  ASSERT(n < std::numeric_limits<vertex_t>::max());
  ASSERT(m < std::numeric_limits<index_t>::max());
  auto fw_head_buffer = FileBuffer::create_r(fw_head_file, (n + 1) * head_bytes);
  auto fw_csr_buffer = FileBuffer::create_r(fw_csr_file, m * csr_bytes);
  auto bw_head_buffer = FileBuffer::create_r(bw_head_file, (n + 1) * head_bytes);
  auto bw_csr_buffer = FileBuffer::create_r(bw_csr_file, m * csr_bytes);

  auto fw_head_access = DenseUnsignedAccessor<>::view(fw_head_buffer.data(), head_bytes, load_endian);
  auto fw_csr_access  = DenseUnsignedAccessor<>::view(fw_csr_buffer.data(), head_bytes, load_endian);
  auto bw_head_access = DenseUnsignedAccessor<>::view(bw_head_buffer.data(), head_bytes, load_endian);
  auto bw_csr_access  = DenseUnsignedAccessor<>::view(bw_csr_buffer.data(), head_bytes, load_endian);

  auto g    = Graph{};
  g.n       = n;
  g.m       = m;
  g.fw_head = borrow<index_t>(memory, n + 1);
  g.fw_csr  = borrow<vertex_t>(memory, m);
  g.bw_head = borrow<index_t>(memory, n + 1);
  g.bw_csr  = borrow<vertex_t>(memory, m);

  for (vertex_t i = 0; i < n + 1; ++i) {
    g.fw_head[i] = fw_head_access.get(i);
  }

  for (vertex_t i = 0; i < m; ++i) {
    g.fw_csr[i] = fw_csr_access.get(i);
  }

  for (vertex_t i = 0; i < n + 1; ++i) {
    g.bw_head[i] = bw_head_access.get(i);
  }

  for (vertex_t i = 0; i < m; ++i) {
    g.bw_csr[i] = bw_csr_access.get(i);
  }

  return g;
}

// template<WorldPartConcept Part>
// static auto
// load_graph_part_from_manifest(Part const& part, Manifest const& manifest, void* memory) -> Result<LocalGraphPart<Part>>
// {
//   auto const  n            = manifest.graph_node_count;
//   auto const  m            = manifest.graph_edge_count;
//   auto const  head_bytes   = manifest.graph_head_bytes;
//   auto const  csr_bytes    = manifest.graph_csr_bytes;
//   auto const  load_endian  = manifest.graph_endian;
//   auto const* fw_head_file = manifest.fw_head_path.c_str();
//   auto const* fw_csr_file  = manifest.fw_csr_path.c_str();
//   auto const* bw_head_file = manifest.bw_head_path.c_str();
//   auto const* bw_csr_file  = manifest.bw_csr_path.c_str();
//
//   RESULT_ASSERT(n < std::numeric_limits<vertex_t>::max(), ASSUMPTION_ERROR);
//   RESULT_ASSERT(m < std::numeric_limits<index_t>::max(), ASSUMPTION_ERROR);
//
//   auto load_dir = [&](char const* head_file, char const* csr_file, auto& graph_head, auto& graph_csr) -> VoidResult {
//     RESULT_TRY(auto head_buffer, FileBuffer::create_r(head_file, (n + 1) * head_bytes));
//     RESULT_TRY(auto csr_buffer, FileBuffer::create_r(csr_file, (n + 1) * csr_bytes));
//
//     auto head_access = DenseUnsignedAccessor<>::view(head_buffer.data(), head_bytes, load_endian);
//     auto csr_access  = DenseUnsignedAccessor<>::view(csr_buffer.data(), csr_bytes, load_endian);
//
//     vertex_t const local_n = part.size();
//     index_t        local_m = 0;
//
//     if constexpr (Part::continuous) {
//       if (local_n > 0)
//         local_m = head_access.get(part.end) - head_access.get(part.begin);
//     } else {
//       for (vertex_t k = 0; k < local_n; ++k) {
//         auto const u = part.select(k);
//         local_m += head_access.get(u + 1) - head_access.get(u);
//       }
//     }
//
//     graph_head = ::borrow<index_t>(memory, local_n);
//     graph_csr  = ::borrow<vertex_t>(memory, local_m);
//
//     u64 pos = 0;
//     for (u64 k = 0; k < local_n; ++k) {
//       auto const index = part.select(k);
//       auto const begin = head_access.get(index);
//       auto const end   = head_access.get(index + 1);
//
//       graph_head.set(k, pos);
//
//       for (auto it = begin; it != end; ++it)
//         graph_csr.set(pos++, csr_access.get(it));
//     }
//     graph_head.set(local_n, pos);
//     return VoidResult::success();
//   };
//
//   auto result        = LocalGraphPart<Part>{};
//   result.buffer      = Buffer::create(2 * page_ceil<index_t>(n + 1), 2 * page_ceil<vertex_t>(m));
//   auto* graph_memory = result.data();
//
//   result.fw_head = borrow<index_t>(graph_memory, n + 1);
//   result.fw_csr  = borrow<vertex_t>(graph_memory, m);
//   result.bw_head = borrow<index_t>(graph_memory, n + 1);
//   result.bw_csr  = borrow<vertex_t>(graph_memory, m);
//
//   RESULT_TRY(load_dir(fw_head_file, fw_csr_file, result.fw_head, result.fw_csr));
//   RESULT_TRY(load_dir(bw_head_file, bw_csr_file, result.bw_head, result.bw_csr));
//
//   result.part = std::move(part);
//   result.n    = n;
//   result.m    = m;
//
//   return result;
// }
//
