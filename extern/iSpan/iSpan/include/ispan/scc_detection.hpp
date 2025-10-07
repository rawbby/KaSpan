#pragma once

#include <comm/MpiBasic.hpp>
#include <graph/Graph.hpp>
#include <ispan/fw_bw_span.hpp>
#include <ispan/get_scc_result.hpp>
#include <ispan/gfq_origin.hpp>
#include <ispan/mice_fw_bw.hpp>
#include <ispan/pivot_selection.hpp>
#include <ispan/process_wcc.hpp>
#include <ispan/trim_1_first.hpp>
#include <ispan/util.hpp>
#include <ispan/wcc_detection.hpp>
#include <util/Time.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mpi.h>
#include <unistd.h>
#include <vector>

inline void
scc_detection(Graph const& graph, int alpha, int world_rank, int world_size, std::vector<index_t>* scc_id_out = nullptr)
{
  auto const n = graph.n;
  auto const m = graph.m;

  auto const* fw_head = static_cast<index_t const*>(graph.fw_head.data());
  auto const* fw_csr  = static_cast<vertex_t const*>(graph.fw_csr.data());
  auto const* bw_head = static_cast<index_t const*>(graph.bw_head.data());
  auto const* bw_csr  = static_cast<vertex_t const*>(graph.bw_csr.data());

  auto* front_comm = new index_t[world_size]{};
  SCOPE_GUARD(delete[] front_comm);

  auto* work_comm = new int[world_size]{};
  SCOPE_GUARD(delete[] work_comm);

  vertex_t s = n / 32;
  if (n % 32 != 0)
    s += 1;

  vertex_t t = s / world_size;
  if (s % world_size != 0)
    t += 1;

  vertex_t const step          = t * 32;
  vertex_t const virtual_count = t * world_size * 32;
  vertex_t const local_beg     = std::min<index_t>(step * world_rank, n);
  vertex_t const local_end     = std::min<index_t>(step * (world_rank + 1), n);

  auto* sa_compress = new unsigned int[s]{};
  SCOPE_GUARD(delete[] sa_compress);

  auto* sub_vertices = new index_t[virtual_count]{};
  SCOPE_GUARD(delete[] sub_vertices);

  auto* sub_wcc_fq = new index_t[virtual_count]{};
  SCOPE_GUARD(delete[] sub_wcc_fq);

  auto* sub_vertices_inverse = new vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_vertices_inverse);

  auto* sub_fw_head = new vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_fw_head);

  auto* sub_fw_csr = new vertex_t[m + 1]{};
  SCOPE_GUARD(delete[] sub_fw_csr);

  auto* sub_bw_head = new vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_bw_head);

  auto* sub_bw_csr = new vertex_t[m + 1]{};
  SCOPE_GUARD(delete[] sub_bw_csr);

  depth_t* fw_sa = nullptr;
  posix_memalign((void**)&fw_sa, getpagesize(), sizeof(*fw_sa) * (virtual_count + 1));

  depth_t* bw_sa = nullptr;
  posix_memalign((void**)&bw_sa, getpagesize(), sizeof(*bw_sa) * (virtual_count + 1));

  auto* fq_comm = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] fq_comm);

  auto* scc_id = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] scc_id);

  auto* sub_scc_id = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] sub_scc_id);

  auto* sub_fw_sa = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] sub_fw_sa);

  auto* sub_wcc_id = new vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] sub_wcc_id);

  for (int i = 0; i < virtual_count + 1; ++i) {
    fw_sa[i]      = depth_unset;
    bw_sa[i]      = depth_unset;
    scc_id[i]     = scc_id_undecided;
    sub_scc_id[i] = scc_id_undecided;
    sub_fw_sa[i]  = scc_id_undecided;
  }

  trim_1_first(scc_id, fw_head, bw_head, local_beg, local_end);

  // clang-format off
  MPI_Allgather(
    /* send: */ MPI_IN_PLACE, 0, mpi_vertex_t,
    /* recv: */ scc_id, step, mpi_vertex_t,
    /* comm: */ MPI_COMM_WORLD);
  // clang-format on

  auto const root = pivot_selection(scc_id, fw_head, bw_head, n);
  fw_span(
    scc_id,
    fw_head,
    bw_head,
    fw_csr,
    bw_csr,
    local_beg,
    local_end,
    fw_sa,
    front_comm,
    root,
    world_rank,
    world_size,
    alpha,
    n,
    m,
    step,
    fq_comm,
    sa_compress);
  memset(sa_compress, 0, s * sizeof(*sa_compress));
  bw_span(
    scc_id,
    fw_head,
    bw_head,
    fw_csr,
    bw_csr,
    local_beg,
    local_end,
    fw_sa,
    bw_sa,
    front_comm,
    work_comm,
    root,
    world_rank,
    world_size,
    alpha,
    n,
    m,
    step,
    fq_comm,
    sa_compress);

  // optional
  // trim_1_normal(scc_id, fw_head, bw_head, fw_csr, bw_csr, local_beg, local_end);
  // trim_1_normal(scc_id, fw_head, bw_head, fw_csr, bw_csr, local_beg, local_end);

  // clang-format off
  MPI_Allreduce(
    MPI_IN_PLACE, scc_id, n, mpi_vertex_t,
    MPI_MIN, MPI_COMM_WORLD);
  // clang-format on

  index_t sub_n = 0;
  gfq_origin(
    // input
    n,
    scc_id,
    fw_head,
    fw_csr,
    bw_head,
    bw_csr,
    // output
    sub_n,
    sub_vertices,
    sub_vertices_inverse,
    sub_fw_head,
    sub_fw_csr,
    sub_bw_head,
    sub_bw_csr);

  if (sub_n > 0) {

    for (index_t i = 0; i < sub_n; ++i)
      sub_wcc_id[i] = i;

    wcc_detection(sub_wcc_id, sub_fw_head, sub_fw_csr, sub_bw_head, sub_bw_csr, sub_n);

    vertex_t sub_wcc_fq_size = 0;
    process_wcc(sub_n, sub_wcc_fq, sub_wcc_id, sub_wcc_fq_size);
    mice_fw_bw(sub_wcc_id, sub_scc_id, sub_fw_head, sub_bw_head, sub_fw_csr, sub_bw_csr, sub_fw_sa, world_rank, world_size, sub_n, sub_wcc_fq, sub_wcc_fq_size, sub_vertices);

    // clang-format off
    MPI_Allreduce(
      MPI_IN_PLACE, sub_scc_id, sub_n, mpi_vertex_t,
      MPI_MIN, MPI_COMM_WORLD);
    // clang-format on

    for (index_t sub_u = 0; sub_u < sub_n; ++sub_u) {
      if (sub_scc_id[sub_u] != scc_id_undecided)
        scc_id[sub_vertices[sub_u]] = sub_scc_id[sub_u];
    }
  }

  get_scc_result(scc_id, n);
  if (scc_id_out != nullptr) {
    scc_id_out->resize(n);
    std::memcpy(scc_id_out->data(), scc_id, n * sizeof(index_t));
  }
}
