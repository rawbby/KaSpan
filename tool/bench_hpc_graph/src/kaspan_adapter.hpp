#pragma once

#include <kaspan/memory/buffer.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <comms.h>
#include <dist_graph.h>
#include <fast_map.h>

#include <cstring>
#include <unordered_set>

// Pure data container for HPCGraph adapter
struct hpc_graph_data
{
  dist_graph_t   g{};
  mpi_data_t     comm{};
  queue_data_t   q{};
  kaspan::buffer storage;
};

// Convert from KaSpan GraphPart to HPCGraph dist_graph_t format
template<kaspan::part_concept Part>
auto
create_hpc_graph_from_graph_part(Part const&           part,
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
  data.g.n_offset    = kaspan::integral_cast<uint64_t>(part.begin()); // Use begin instead of local_start()
  data.g.n_ghost     = 0;
  data.g.n_total     = data.g.n_local; // no ghost vertices for now
  data.g.m_local_out = kaspan::integral_cast<uint64_t>(local_fw_m);
  data.g.m_local_in  = kaspan::integral_cast<uint64_t>(local_bw_m);

  // Allocate and convert forward edges (out_edges)
  auto out_degree_list_buf = kaspan::make_buffer<uint64_t>(local_n + 1);
  auto out_edges_buf       = kaspan::make_buffer<uint64_t>(local_fw_m);

  auto* out_degree_list = static_cast<uint64_t*>(out_degree_list_buf.data());
  auto* out_edges       = static_cast<uint64_t*>(out_edges_buf.data());

  // Convert forward CSR
  for (kaspan::vertex_t i = 0; i <= local_n; ++i) {
    out_degree_list[i] = kaspan::integral_cast<uint64_t>(fw_head[i]);
  }

  for (kaspan::index_t i = 0; i < local_fw_m; ++i) {
    out_edges[i] = kaspan::integral_cast<uint64_t>(fw_csr[i]);
  }

  // Allocate and convert backward edges (in_edges)
  auto in_degree_list_buf = kaspan::make_buffer<uint64_t>(local_n + 1);
  auto in_edges_buf       = kaspan::make_buffer<uint64_t>(local_bw_m);

  auto* in_degree_list = static_cast<uint64_t*>(in_degree_list_buf.data());
  auto* in_edges       = static_cast<uint64_t*>(in_edges_buf.data());

  // Convert backward CSR
  for (kaspan::vertex_t i = 0; i <= local_n; ++i) {
    in_degree_list[i] = kaspan::integral_cast<uint64_t>(bw_head[i]);
  }

  for (kaspan::index_t i = 0; i < local_bw_m; ++i) {
    in_edges[i] = kaspan::integral_cast<uint64_t>(bw_csr[i]);
  }

  // Create local_unmap (maps local index to global vertex id)
  auto  local_unmap_buf = kaspan::make_buffer<uint64_t>(local_n);
  auto* local_unmap     = static_cast<uint64_t*>(local_unmap_buf.data());

  for (kaspan::vertex_t i = 0; i < local_n; ++i) {
    local_unmap[i] = kaspan::integral_cast<uint64_t>(part.to_global(i));
  }

  // Temporarily assign pointers
  data.g.out_degree_list = out_degree_list;
  data.g.out_edges       = out_edges;
  data.g.in_degree_list  = in_degree_list;
  data.g.in_edges        = in_edges;
  data.g.local_unmap     = local_unmap;

  // Ghost cells will be initialized separately via initialize_ghost_cells()
  // For now, set ghost_unmap and ghost_tasks to nullptr
  data.g.ghost_unmap = nullptr;
  data.g.ghost_tasks = nullptr;

  // Find max out and in degree for statistics
  data.g.max_out_degree = 0;
  data.g.max_in_degree  = 0;

  for (uint64_t i = 0; i < data.g.n_local; ++i) {
    uint64_t const out_deg = out_degree_list[i + 1] - out_degree_list[i];
    uint64_t const in_deg  = in_degree_list[i + 1] - in_degree_list[i];

    if (out_deg > data.g.max_out_degree) {
      data.g.max_out_degree = out_deg;
    }
    if (in_deg > data.g.max_in_degree) {
      data.g.max_in_degree = in_deg;
    }
  }

  // Store buffer for ownership (just use one buffer, others will be freed at end of function)
  // The pointers in data.g point to memory owned by the buffers that will be destroyed
  // We need to keep ALL buffers alive, so we combine their sizes
  // Note: Ghost arrays will be added later by initialize_ghost_cells()
  size_t total_size = (local_n + 1) * sizeof(uint64_t)   // out_degree_list
                      + local_fw_m * sizeof(uint64_t)    // out_edges
                      + (local_n + 1) * sizeof(uint64_t) // in_degree_list
                      + local_bw_m * sizeof(uint64_t)    // in_edges
                      + local_n * sizeof(uint64_t);      // local_unmap

  // Create a single combined buffer to hold all data
  data.storage         = kaspan::make_buffer<uint64_t>(total_size / sizeof(uint64_t));
  char*  combined_data = static_cast<char*>(data.storage.data());
  size_t offset        = 0;

  // Copy all data into the combined buffer and update pointers
  std::memcpy(combined_data + offset, out_degree_list, (local_n + 1) * sizeof(uint64_t));
  data.g.out_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);

  std::memcpy(combined_data + offset, out_edges, local_fw_m * sizeof(uint64_t));
  data.g.out_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_fw_m * sizeof(uint64_t);

  std::memcpy(combined_data + offset, in_degree_list, (local_n + 1) * sizeof(uint64_t));
  data.g.in_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);

  std::memcpy(combined_data + offset, in_edges, local_bw_m * sizeof(uint64_t));
  data.g.in_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_bw_m * sizeof(uint64_t);

  std::memcpy(combined_data + offset, local_unmap, local_n * sizeof(uint64_t));
  data.g.local_unmap = reinterpret_cast<uint64_t*>(combined_data + offset);

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
initialize_ghost_cells(hpc_graph_data& data, Part const& part, kaspan::index_t local_fw_m, kaspan::index_t local_bw_m)
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
create_hpc_graph_from_local_graph_part(Part const&           part,
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
destroy_hpc_graph_data(hpc_graph_data* data)
{
  clear_map(&data->g.map);
  clear_comm_data(&data->comm);
  clear_queue_data(&data->q);
}
