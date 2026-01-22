#pragma once

#include <kaspan/graph/concept.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <comms.h>
#include <dist_graph.h>

#include <cstring>

struct hpc_graph_data
{
  dist_graph_t g;
  mpi_data_t   comm;
  queue_data_t q;

  bool initialized = false;

  hpc_graph_data() noexcept
  {
    g.n                    = 0;
    g.m                    = 0;
    g.m_local_out          = 0;
    g.m_local_in           = 0;
    g.n_local              = 0;
    g.n_offset             = 0;
    g.n_ghost              = 0;
    g.n_total              = 0;
    g.max_degree_vert      = 0;
    g.max_out_degree       = 0;
    g.max_in_degree        = 0;
    g.out_edges            = nullptr;
    g.in_edges             = nullptr;
    g.out_degree_list      = nullptr;
    g.in_degree_list       = nullptr;
    g.local_unmap          = nullptr;
    g.ghost_unmap          = nullptr;
    g.ghost_tasks          = nullptr;
    g.map.arr              = nullptr;
    g.map.unique_keys      = nullptr;
    g.map.unique_indexes   = nullptr;
    g.map.capacity         = 0;
    g.map.num_unique       = 0;
    g.map.hashing          = false;
    comm.sendcounts        = nullptr;
    comm.recvcounts        = nullptr;
    comm.sdispls           = nullptr;
    comm.rdispls           = nullptr;
    comm.sdispls_cpy       = nullptr;
    comm.recvcounts_temp   = nullptr;
    comm.sendcounts_temp   = nullptr;
    comm.sdispls_temp      = nullptr;
    comm.rdispls_temp      = nullptr;
    comm.sdispls_cpy_temp  = nullptr;
    comm.sendbuf_vert      = nullptr;
    comm.sendbuf_data      = nullptr;
    comm.sendbuf_data_flt  = nullptr;
    comm.recvbuf_vert      = nullptr;
    comm.recvbuf_data      = nullptr;
    comm.recvbuf_data_flt  = nullptr;
    comm.total_recv        = 0;
    comm.total_send        = 0;
    comm.global_queue_size = 0;
    q.queue                = nullptr;
    q.queue_next           = nullptr;
    q.queue_send           = nullptr;
    q.queue_size           = 0;
    q.next_size            = 0;
    q.send_size            = 0;
  }

  ~hpc_graph_data()
  {
    kaspan::line_free(g.out_edges);
    kaspan::line_free(g.in_edges);
    kaspan::line_free(g.out_degree_list);
    kaspan::line_free(g.in_degree_list);
    kaspan::line_free(g.local_unmap);
    kaspan::line_free(g.ghost_unmap);
    kaspan::line_free(g.ghost_tasks);

    if (initialized) {
      clear_map(&g.map);
      clear_comm_data(&comm);
      clear_queue_data(&q);
    }
  }

  hpc_graph_data(hpc_graph_data const&) = delete;
  hpc_graph_data(
    hpc_graph_data&& rhs) noexcept
  {
    g           = rhs.g;
    comm        = rhs.comm;
    q           = rhs.q;
    initialized = rhs.initialized;
    new (&rhs) hpc_graph_data{};
  }

  auto operator=(hpc_graph_data const&) -> hpc_graph_data& = delete;
  auto operator=(
    hpc_graph_data&& rhs) noexcept -> hpc_graph_data&
  {
    if (this != &rhs) {
      this->~hpc_graph_data();
      new (this) hpc_graph_data{std::move(rhs)};
    }
    return *this;
  }

  void initialize()
  {
    if (!initialized) {
      using namespace kaspan;

      // todo ? init these after all other graph info was written
      g.n_ghost         = 0;
      g.max_degree_vert = 0;
      g.max_out_degree  = 0;
      g.max_in_degree   = 0;
      g.ghost_unmap     = nullptr;
      g.ghost_tasks     = nullptr;

      g.m = integral_cast<u64>(mpi_basic::allreduce_single(g.m_local_out, mpi_basic::sum));

      g.n_total     = g.n_local;
      g.local_unmap = line_alloc<u64>(g.n_local);
      for (uint64_t i = 0; i < g.n_local; ++i)
        g.local_unmap[i] = g.n_offset + i;

      init_map(&g.map, g.n_local * 2);
      for (uint64_t i = 0; i < g.n_local; ++i)
        set_value_uq(&g.map, g.local_unmap[i], i);

      init_comm_data(&comm);
      init_queue_data(&g, &q);

      initialized = true;
    }
  }
};

template<kaspan::part_concept Part>
void
initialize_ghost_cells(
  hpc_graph_data& data,
  Part const&     part,
  uint64_t        local_fw_m,
  uint64_t        local_bw_m)
{
  using namespace kaspan;
  dist_graph_t& g = data.g;

  for (uint64_t i = 0; i < local_fw_m; ++i) {
    uint64_t v   = g.out_edges[i];
    uint64_t val = get_value(&g.map, v);
    if (val == NULL_KEY) {
      set_value_uq(&g.map, v, g.n_local + g.n_ghost);
      g.n_ghost++;
    }
  }
  for (uint64_t i = 0; i < local_bw_m; ++i) {
    uint64_t v   = g.in_edges[i];
    uint64_t val = get_value(&g.map, v);
    if (val == NULL_KEY) {
      set_value_uq(&g.map, v, g.n_local + g.n_ghost);
      g.n_ghost++;
    }
  }

  g.n_total = g.n_local + g.n_ghost;

  if (g.n_ghost > 0) {
    g.ghost_unmap = line_alloc<u64>(g.n_ghost);
    g.ghost_tasks = line_alloc<u64>(g.n_ghost);
    for (uint64_t i = 0; i < g.n_ghost; ++i) {
      uint64_t global_v = g.map.unique_keys[g.n_local + i];
      g.ghost_unmap[i]  = global_v;
      g.ghost_tasks[i]  = integral_cast<u64>(part.world_rank_of(integral_cast<vertex_t>(global_v)));
    }
  }

  for (uint64_t i = 0; i < local_fw_m; ++i) {
    g.out_edges[i] = get_value(&g.map, g.out_edges[i]);
  }
  for (uint64_t i = 0; i < local_bw_m; ++i) {
    g.in_edges[i] = get_value(&g.map, g.in_edges[i]);
  }
}
