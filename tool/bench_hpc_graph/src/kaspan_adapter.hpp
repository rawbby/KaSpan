#pragma once

#include <mpi.h>

#include <debug/assert.hpp>
#include <memory/buffer.hpp>
#include <scc/graph.hpp>

#include "comms.h"
#include "dist_graph.h"
#include "fast_map.h"

#include <cstring>
#include <unordered_set>

// Pure data container for HPCGraph adapter
struct HPCGraphData
{
  dist_graph_t g;
  mpi_data_t   comm;
  queue_data_t q;
  Buffer       buffer;
};

// Convert from KaSpan GraphPart to HPCGraph dist_graph_t format
template<WorldPartConcept Part>
auto
create_hpc_graph_from_graph_part(GraphPart<Part> const& graph_part) -> HPCGraphData
{
  HPCGraphData data;

  auto const& part      = graph_part.part;
  vertex_t const local_n = part.local_n();
  vertex_t const n       = part.n;

  // Initialize dist_graph_t structure
  data.g.n           = static_cast<uint64_t>(n);
  data.g.m           = static_cast<uint64_t>(graph_part.m);
  data.g.n_local     = static_cast<uint64_t>(local_n);
  data.g.n_offset    = static_cast<uint64_t>(part.begin); // Use begin instead of local_start()
  data.g.n_ghost     = 0;
  data.g.n_total     = data.g.n_local; // no ghost vertices for now
  data.g.m_local_out = static_cast<uint64_t>(graph_part.local_fw_m);
  data.g.m_local_in  = static_cast<uint64_t>(graph_part.local_bw_m);

  // Allocate and convert forward edges (out_edges)
  auto out_degree_list_buf = make_buffer<uint64_t>(local_n + 1);
  auto out_edges_buf       = make_buffer<uint64_t>(graph_part.local_fw_m);

  uint64_t* out_degree_list = static_cast<uint64_t*>(out_degree_list_buf.data());
  uint64_t* out_edges       = static_cast<uint64_t*>(out_edges_buf.data());

  // Convert forward CSR
  for (vertex_t i = 0; i <= local_n; ++i) {
    out_degree_list[i] = static_cast<uint64_t>(graph_part.fw_head[i]);
  }

  for (index_t i = 0; i < graph_part.local_fw_m; ++i) {
    out_edges[i] = static_cast<uint64_t>(graph_part.fw_csr[i]);
  }

  // Allocate and convert backward edges (in_edges)
  auto in_degree_list_buf = make_buffer<uint64_t>(local_n + 1);
  auto in_edges_buf       = make_buffer<uint64_t>(graph_part.local_bw_m);

  uint64_t* in_degree_list = static_cast<uint64_t*>(in_degree_list_buf.data());
  uint64_t* in_edges       = static_cast<uint64_t*>(in_edges_buf.data());

  // Convert backward CSR
  for (vertex_t i = 0; i <= local_n; ++i) {
    in_degree_list[i] = static_cast<uint64_t>(graph_part.bw_head[i]);
  }

  for (index_t i = 0; i < graph_part.local_bw_m; ++i) {
    in_edges[i] = static_cast<uint64_t>(graph_part.bw_csr[i]);
  }

  // Create local_unmap (maps local index to global vertex id)
  auto      local_unmap_buf = make_buffer<uint64_t>(local_n);
  uint64_t* local_unmap     = static_cast<uint64_t*>(local_unmap_buf.data());

  for (vertex_t i = 0; i < local_n; ++i) {
    local_unmap[i] = static_cast<uint64_t>(part.to_global(i));
  }

  // Temporarily assign pointers
  data.g.out_degree_list = out_degree_list;
  data.g.out_edges       = out_edges;
  data.g.in_degree_list  = in_degree_list;
  data.g.in_edges        = in_edges;
  data.g.local_unmap     = local_unmap;

  // Find all ghost vertices (remote vertices referenced in edges)
  std::unordered_set<uint64_t> ghost_set;
  for (index_t i = 0; i < graph_part.local_fw_m; ++i) {
    uint64_t global_id = out_edges[i];
    // Check if it's not a local vertex
    if (global_id < part.begin || global_id >= part.end) {
      ghost_set.insert(global_id);
    }
  }
  for (index_t i = 0; i < graph_part.local_bw_m; ++i) {
    uint64_t global_id = in_edges[i];
    // Check if it's not a local vertex
    if (global_id < part.begin || global_id >= part.end) {
      ghost_set.insert(global_id);
    }
  }

  // Create ghost_unmap array
  vertex_t n_ghost = static_cast<vertex_t>(ghost_set.size());
  auto ghost_unmap_buf = make_buffer<uint64_t>(std::max(n_ghost, vertex_t(1)));
  auto ghost_tasks_buf = make_buffer<uint64_t>(std::max(n_ghost, vertex_t(1)));

  uint64_t* ghost_unmap = static_cast<uint64_t*>(ghost_unmap_buf.data());
  uint64_t* ghost_tasks = static_cast<uint64_t*>(ghost_tasks_buf.data());

  uint64_t ghost_idx = 0;
  for (auto global_id : ghost_set) {
    ghost_unmap[ghost_idx] = global_id;
    ghost_tasks[ghost_idx] = 0; // Will be set later by HPC graph library
    ++ghost_idx;
  }

  data.g.n_ghost = static_cast<uint64_t>(n_ghost);
  data.g.n_total = data.g.n_local + data.g.n_ghost;
  data.g.ghost_unmap = ghost_unmap;
  data.g.ghost_tasks = ghost_tasks;

  // Find max out and in degree for statistics
  data.g.max_out_degree = 0;
  data.g.max_in_degree  = 0;

  for (uint64_t i = 0; i < data.g.n_local; ++i) {
    uint64_t out_deg = out_degree_list[i + 1] - out_degree_list[i];
    uint64_t in_deg  = in_degree_list[i + 1] - in_degree_list[i];

    if (out_deg > data.g.max_out_degree)
      data.g.max_out_degree = out_deg;
    if (in_deg > data.g.max_in_degree)
      data.g.max_in_degree = in_deg;
  }

  // Store buffer for ownership (just use one buffer, others will be freed at end of function)
  // The pointers in data.g point to memory owned by the buffers that will be destroyed
  // We need to keep ALL buffers alive, so we combine their sizes
  size_t total_size = (local_n + 1) * sizeof(uint64_t)        // out_degree_list
                      + graph_part.local_fw_m * sizeof(uint64_t) // out_edges
                      + (local_n + 1) * sizeof(uint64_t)         // in_degree_list
                      + graph_part.local_bw_m * sizeof(uint64_t) // in_edges
                      + local_n * sizeof(uint64_t)               // local_unmap
                      + std::max(n_ghost, vertex_t(1)) * sizeof(uint64_t)  // ghost_unmap
                      + std::max(n_ghost, vertex_t(1)) * sizeof(uint64_t); // ghost_tasks

  // Create a single combined buffer to hold all data
  data.buffer          = make_buffer<uint64_t>(total_size / sizeof(uint64_t));
  char*  combined_data = static_cast<char*>(data.buffer.data());
  size_t offset        = 0;

  // Copy all data into the combined buffer and update pointers
  std::memcpy(combined_data + offset, out_degree_list, (local_n + 1) * sizeof(uint64_t));
  data.g.out_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);

  std::memcpy(combined_data + offset, out_edges, graph_part.local_fw_m * sizeof(uint64_t));
  data.g.out_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += graph_part.local_fw_m * sizeof(uint64_t);

  std::memcpy(combined_data + offset, in_degree_list, (local_n + 1) * sizeof(uint64_t));
  data.g.in_degree_list = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += (local_n + 1) * sizeof(uint64_t);

  std::memcpy(combined_data + offset, in_edges, graph_part.local_bw_m * sizeof(uint64_t));
  data.g.in_edges = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += graph_part.local_bw_m * sizeof(uint64_t);

  std::memcpy(combined_data + offset, local_unmap, local_n * sizeof(uint64_t));
  data.g.local_unmap = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += local_n * sizeof(uint64_t);

  size_t ghost_size = std::max(n_ghost, vertex_t(1)) * sizeof(uint64_t);
  std::memcpy(combined_data + offset, ghost_unmap_buf.data(), ghost_size);
  data.g.ghost_unmap = reinterpret_cast<uint64_t*>(combined_data + offset);
  offset += ghost_size;

  std::memcpy(combined_data + offset, ghost_tasks_buf.data(), ghost_size);
  data.g.ghost_tasks = reinterpret_cast<uint64_t*>(combined_data + offset);

  // Initialize fast_map AFTER copying data to final location
  // (required by HPCGraph for vertex lookups)
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
    uint64_t global_id = data.g.out_edges[i];
    uint64_t local_idx = get_value(&data.g.map, global_id);
    data.g.out_edges[i] = local_idx;
  }
  for (uint64_t i = 0; i < data.g.m_local_in; ++i) {
    uint64_t global_id = data.g.in_edges[i];
    uint64_t local_idx = get_value(&data.g.map, global_id);
    data.g.in_edges[i] = local_idx;
  }

  // Initialize communication structures
  init_comm_data(&data.comm);
  init_queue_data(&data.g, &data.q);

  return data;
}

// Convert from KaSpan LocalGraphPart to HPCGraph dist_graph_t format
template<WorldPartConcept Part>
auto
create_hpc_graph_from_local_graph_part(LocalGraphPart<Part> const& graph_part) -> HPCGraphData
{
  // Construct non-owning GraphPart view and delegate
  GraphPart<Part> view{ graph_part };
  return create_hpc_graph_from_graph_part(view);
}

// Cleanup function for HPCGraphData
inline void
destroy_hpc_graph_data(HPCGraphData* data)
{
  clear_map(&data->g.map);
  clear_comm_data(&data->comm);
  clear_queue_data(&data->q);
}
