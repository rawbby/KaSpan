#pragma once

// Convert from KaSpan GraphPart to HPCGraph dist_graph_t format
template<kaspan::part_concept Part>
auto
create_hpc_graph_from_graph_part(
  Part const&             part,
  kaspan::index_t         m,
  kaspan::index_t         local_fw_m,
  kaspan::index_t         local_bw_m,
  kaspan::index_t const*  fw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::index_t const*  bw_head,
  kaspan::vertex_t const* bw_csr) -> hpc_graph_data
{
  hpc_graph_data data;

  kaspan::vertex_t const local_n = part.local_n();
  kaspan::vertex_t const n       = part.n();

  // Initialize dist_graph_t structure
  data.g.n           = kaspan::integral_cast<uint64_t>(n);
  data.g.m           = kaspan::integral_cast<uint64_t>(m);
  data.g.n_local     = kaspan::integral_cast<uint64_t>(local_n);
  data.g.n_offset    = kaspan::integral_cast<uint64_t>(part.begin());
  data.g.n_ghost     = 0;
  data.g.n_total     = data.g.n_local;
  data.g.m_local_out = kaspan::integral_cast<uint64_t>(local_fw_m);
  data.g.m_local_in  = kaspan::integral_cast<uint64_t>(local_bw_m);

  size_t total_size = (local_n + 1) * sizeof(uint64_t)   // out_degree_list
                      + local_fw_m * sizeof(uint64_t)    // out_edges
                      + (local_n + 1) * sizeof(uint64_t) // in_degree_list
                      + local_bw_m * sizeof(uint64_t)    // in_edges
                      + local_n * sizeof(uint64_t);      // local_unmap

  // Create a single combined buffer to hold all data
  data.storage         = kaspan::make_buffer<uint64_t>(total_size / sizeof(uint64_t));
  char*  combined_data = static_cast<char*>(data.storage.data());
  size_t offset        = 0;

  data.g.out_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);

  data.g.out_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_fw_m * sizeof(uint64_t);

  data.g.in_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);

  data.g.in_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_bw_m * sizeof(uint64_t);

  data.g.local_unmap = reinterpret_cast<uint64_t*>(combined_data + offset);

  // Convert and fill data
  data.g.max_out_degree = 0;
  data.g.max_in_degree  = 0;

  for (kaspan::vertex_t i = 0; i <= local_n; ++i) {
    data.g.out_degree_list[i] = kaspan::integral_cast<uint64_t>(fw_head[i]);
  }
  for (kaspan::index_t i = 0; i < local_fw_m; ++i) {
    data.g.out_edges[i] = kaspan::integral_cast<uint64_t>(fw_csr[i]);
  }
  for (kaspan::vertex_t i = 0; i <= local_n; ++i) {
    data.g.in_degree_list[i] = kaspan::integral_cast<uint64_t>(bw_head[i]);
  }
  for (kaspan::index_t i = 0; i < local_bw_m; ++i) {
    data.g.in_edges[i] = kaspan::integral_cast<uint64_t>(bw_csr[i]);
  }
  for (kaspan::vertex_t i = 0; i < local_n; ++i) {
    data.g.local_unmap[i] = kaspan::integral_cast<uint64_t>(part.to_global(i));

    uint64_t const out_deg = data.g.out_degree_list[i + 1] - data.g.out_degree_list[i];
    uint64_t const in_deg  = data.g.in_degree_list[i + 1] - data.g.in_degree_list[i];

    if (out_deg > data.g.max_out_degree) data.g.max_out_degree = out_deg;
    if (in_deg > data.g.max_in_degree) data.g.max_in_degree = in_deg;
  }

  // Initialize communication structures
  init_comm_data(&data.comm);
  init_queue_data(&data.g, &data.q);

  return data;
}

// Load HPCGraph data directly from manifest without intermediate KaSpan graph
template<kaspan::part_concept Part>
auto
load_hpc_graph_from_manifest(
  Part const&             part,
  kaspan::manifest const& manifest) -> hpc_graph_data
{
  using namespace kaspan;
  hpc_graph_data data;

  vertex_t const n       = part.n();
  vertex_t const local_n = part.local_n();
  index_t const  m       = integral_cast<index_t>(manifest.graph_edge_count);

  auto const  head_bytes   = manifest.graph_head_bytes;
  auto const  csr_bytes    = manifest.graph_csr_bytes;
  auto const  load_endian  = manifest.graph_endian;
  char const* fw_head_file = manifest.fw_head_path.c_str();
  char const* fw_csr_file  = manifest.fw_csr_path.c_str();
  char const* bw_head_file = manifest.bw_head_path.c_str();
  char const* bw_csr_file  = manifest.bw_csr_path.c_str();

  auto fw_head_buffer = file_buffer::create_r(fw_head_file, (n + 1) * head_bytes);
  auto fw_csr_buffer  = file_buffer::create_r(fw_csr_file, m * csr_bytes);
  auto bw_head_buffer = file_buffer::create_r(bw_head_file, (n + 1) * head_bytes);
  auto bw_csr_buffer  = file_buffer::create_r(bw_csr_file, m * csr_bytes);

  auto fw_head_access = view_dense_unsigned(fw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto fw_csr_access  = view_dense_unsigned(fw_csr_buffer.data(), m, csr_bytes, load_endian);
  auto bw_head_access = view_dense_unsigned(bw_head_buffer.data(), n + 1, head_bytes, load_endian);
  auto bw_csr_access  = view_dense_unsigned(bw_csr_buffer.data(), m, csr_bytes, load_endian);

  auto const get_local_m = [&](auto const& head) -> index_t {
    if constexpr (Part::continuous()) {
      if (local_n > 0) {
        return integral_cast<index_t>(head.get(part.end()) - head.get(part.begin()));
      }
      return 0;
    } else {
      index_t sum = 0;
      for (vertex_t k = 0; k < local_n; ++k) {
        auto const u = part.select(k);
        sum += integral_cast<index_t>(head.get(u + 1) - head.get(u));
      }
      return sum;
    }
  };

  index_t const local_fw_m = get_local_m(fw_head_access);
  index_t const local_bw_m = get_local_m(bw_head_access);

  // Initialize dist_graph_t structure
  data.g.n           = integral_cast<uint64_t>(n);
  data.g.m           = integral_cast<uint64_t>(m);
  data.g.n_local     = integral_cast<uint64_t>(local_n);
  data.g.n_offset    = integral_cast<uint64_t>(part.begin());
  data.g.n_ghost     = 0;
  data.g.n_total     = data.g.n_local;
  data.g.m_local_out = integral_cast<uint64_t>(local_fw_m);
  data.g.m_local_in  = integral_cast<uint64_t>(local_bw_m);

  size_t total_size = (local_n + 1) * sizeof(uint64_t)   // out_degree_list
                      + local_fw_m * sizeof(uint64_t)    // out_edges
                      + (local_n + 1) * sizeof(uint64_t) // in_degree_list
                      + local_bw_m * sizeof(uint64_t)    // in_edges
                      + local_n * sizeof(uint64_t);      // local_unmap

  data.storage         = make_buffer<uint64_t>(total_size / sizeof(uint64_t));
  char*  combined_data = static_cast<char*>(data.storage.data());
  size_t offset        = 0;

  data.g.out_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);
  data.g.out_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_fw_m * sizeof(uint64_t);
  data.g.in_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);
  data.g.in_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_bw_m * sizeof(uint64_t);
  data.g.local_unmap = reinterpret_cast<uint64_t*>(combined_data + offset);

  // Fill data
  uint64_t fw_pos = 0;
  uint64_t bw_pos = 0;
  data.g.max_out_degree = 0;
  data.g.max_in_degree  = 0;

  for (vertex_t k = 0; k < local_n; ++k) {
    auto const global_u   = part.to_global(k);
    data.g.local_unmap[k] = integral_cast<uint64_t>(global_u);

    // Forward
    auto const fw_begin       = fw_head_access.get(global_u);
    auto const fw_end         = fw_head_access.get(global_u + 1);
    data.g.out_degree_list[k] = fw_pos;
    for (auto it = fw_begin; it != fw_end; ++it) {
      data.g.out_edges[fw_pos++] = integral_cast<uint64_t>(fw_csr_access.get(it));
    }
    uint64_t const out_deg = fw_pos - data.g.out_degree_list[k];
    if (out_deg > data.g.max_out_degree) data.g.max_out_degree = out_deg;

    // Backward
    auto const bw_begin      = bw_head_access.get(global_u);
    auto const bw_end        = bw_head_access.get(global_u + 1);
    data.g.in_degree_list[k] = bw_pos;
    for (auto it = bw_begin; it != bw_end; ++it) {
      data.g.in_edges[bw_pos++] = integral_cast<uint64_t>(bw_csr_access.get(it));
    }
    uint64_t const in_deg = bw_pos - data.g.in_degree_list[k];
    if (in_deg > data.g.max_in_degree) data.g.max_in_degree = in_deg;
  }
  if (local_n > 0) {
    data.g.out_degree_list[local_n] = fw_pos;
    data.g.in_degree_list[local_n]  = bw_pos;
  } else {
    data.g.out_degree_list[0] = 0;
    data.g.in_degree_list[0]  = 0;
  }

  // Initialize communication structures
  init_comm_data(&data.comm);
  init_queue_data(&data.g, &data.q);

  return data;
}

// Initialize ghost cells for HPCGraph adapter
// This is an accelerator structure specific to HPCGraph and should be counted
// as part of the algorithm benchmark time, not as graph conversion overhead
template<kaspan::part_concept Part>
void
initialize_ghost_cells(
  hpc_graph_data& data,
  Part const&     part,
  kaspan::index_t local_fw_m,
  kaspan::index_t local_bw_m)
{
  kaspan::vertex_t const local_n = part.local_n();

  // Find all ghost vertices (remote vertices referenced in edges)
  std::unordered_set<uint64_t> ghost_set;
  for (kaspan::index_t i = 0; i < local_fw_m; ++i) {
    uint64_t global_id = data.g.out_edges[i];
    // Check if it's not a local vertex
    if (global_id < part.begin() || global_id >= part.end()) {
      ghost_set.insert(global_id);
    }
  }
  for (kaspan::index_t i = 0; i < local_bw_m; ++i) {
    uint64_t global_id = data.g.in_edges[i];
    // Check if it's not a local vertex
    if (global_id < part.begin() || global_id >= part.end()) {
      ghost_set.insert(global_id);
    }
  }

  // Create ghost_unmap and ghost_tasks arrays
  auto n_ghost         = kaspan::integral_cast<kaspan::vertex_t>(ghost_set.size());
  auto ghost_unmap_buf = kaspan::make_buffer<uint64_t>(std::max(n_ghost, kaspan::vertex_t(1)));
  auto ghost_tasks_buf = kaspan::make_buffer<uint64_t>(std::max(n_ghost, kaspan::vertex_t(1)));

  auto* ghost_unmap = static_cast<uint64_t*>(ghost_unmap_buf.data());
  auto* ghost_tasks = static_cast<uint64_t*>(ghost_tasks_buf.data());

  uint64_t ghost_idx = 0;
  for (auto global_id : ghost_set) {
    ghost_unmap[ghost_idx] = global_id;
    ghost_tasks[ghost_idx] = kaspan::integral_cast<uint64_t>(part.world_rank_of(kaspan::integral_cast<kaspan::vertex_t>(global_id)));
    ++ghost_idx;
  }

  // Update graph structure with ghost information
  data.g.n_ghost = kaspan::integral_cast<uint64_t>(n_ghost);
  data.g.n_total = data.g.n_local + data.g.n_ghost;

  // Need to extend the buffer to include ghost arrays
  size_t ghost_size      = std::max(n_ghost, kaspan::vertex_t(1)) * sizeof(uint64_t);
  size_t old_buffer_size = (local_n + 1) * sizeof(uint64_t)   // out_degree_list
                           + local_fw_m * sizeof(uint64_t)    // out_edges
                           + (local_n + 1) * sizeof(uint64_t) // in_degree_list
                           + local_bw_m * sizeof(uint64_t)    // in_edges
                           + local_n * sizeof(uint64_t);      // local_unmap
  size_t const new_buffer_size = old_buffer_size + 2 * ghost_size;

  // Create new buffer with space for ghost arrays
  auto new_buffer = kaspan::make_buffer<uint64_t>(new_buffer_size / sizeof(uint64_t));

  // Copy existing data to new buffer
  std::memcpy(new_buffer.data(), data.storage.data(), old_buffer_size);

  // Calculate offsets for existing pointers
  char* old_base = static_cast<char*>(data.storage.data());
  char* new_base = static_cast<char*>(new_buffer.data());

  // Update pointers to point to new buffer
  data.g.out_degree_list = reinterpret_cast<uint64_t*>(new_base + (reinterpret_cast<char*>(data.g.out_degree_list) - old_base));
  data.g.out_edges       = reinterpret_cast<uint64_t*>(new_base + (reinterpret_cast<char*>(data.g.out_edges) - old_base));
  data.g.in_degree_list  = reinterpret_cast<uint64_t*>(new_base + (reinterpret_cast<char*>(data.g.in_degree_list) - old_base));
  data.g.in_edges        = reinterpret_cast<uint64_t*>(new_base + (reinterpret_cast<char*>(data.g.in_edges) - old_base));
  data.g.local_unmap     = reinterpret_cast<uint64_t*>(new_base + (reinterpret_cast<char*>(data.g.local_unmap) - old_base));

  // Add ghost arrays at the end
  size_t offset = old_buffer_size;
  std::memcpy(new_base + offset, ghost_unmap, ghost_size);
  data.g.ghost_unmap = reinterpret_cast<uint64_t*>(new_base + offset);
  offset += ghost_size;

  std::memcpy(new_base + offset, ghost_tasks, ghost_size);
  data.g.ghost_tasks = reinterpret_cast<uint64_t*>(new_base + offset);

  // Replace old buffer with new one
  data.storage = std::move(new_buffer);

  // Initialize fast_map (required by HPCGraph for vertex lookups)
  // Map size needs to accommodate both local and ghost vertices
  init_map(&data.g.map, data.g.n_total * 2);

  // Add local vertices to map
  for (uint64_t i = 0; i < data.g.n_local; ++i) {
    set_value(&data.g.map, data.g.local_unmap[i], i);
  }

  // Add ghost vertices to map
  for (uint64_t i = 0; i < data.g.n_ghost; ++i) {
    set_value(&data.g.map, data.g.ghost_unmap[i], data.g.n_local + i);
  }

  // Convert edge arrays from global IDs to local/ghost indices
  for (uint64_t i = 0; i < data.g.m_local_out; ++i) {
    uint64_t const global_id = data.g.out_edges[i];
    uint64_t const local_idx = get_value(&data.g.map, global_id);
    data.g.out_edges[i]      = local_idx;
  }
  for (uint64_t i = 0; i < data.g.m_local_in; ++i) {
    uint64_t const global_id = data.g.in_edges[i];
    uint64_t const local_idx = get_value(&data.g.map, global_id);
    data.g.in_edges[i]       = local_idx;
  }
}

// Convert from KaSpan local_graph_part to HPCGraph dist_graph_t format
template<kaspan::part_concept Part>
auto
create_hpc_graph_from_local_graph_part(
  Part const&             part,
  kaspan::index_t         m,
  kaspan::index_t         local_fw_m,
  kaspan::index_t         local_bw_m,
  kaspan::index_t const*  fw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::index_t const*  bw_head,
  kaspan::vertex_t const* bw_csr) -> hpc_graph_data
{
  return create_hpc_graph_from_graph_part(part, m, local_fw_m, local_bw_m, fw_head, fw_csr, bw_head, bw_csr);
}

// Cleanup function for HPCGraphData
inline void
destroy_hpc_graph_data(
  hpc_graph_data* data)
{
  clear_map(&data->g.map);
  clear_comm_data(&data->comm);
  clear_queue_data(&data->q);
}
