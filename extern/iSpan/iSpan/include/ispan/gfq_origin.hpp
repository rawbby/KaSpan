#pragma once

#include <ispan/util.hpp>

static void
gfq_origin(
  index_t const   n,
  index_t const*  scc_id,
  index_t const*  fw_beg,
  vertex_t const* fw_csr,
  index_t const*  bw_beg,
  vertex_t const* bw_csr,

  vertex_t& sub_n,
  vertex_t* sub_vertices,
  vertex_t* sub_vertices_inverse,
  vertex_t* sub_fw_beg,
  vertex_t* sub_fw_csr,
  vertex_t* sub_bw_beg,
  vertex_t* sub_bw_csr)
{
  sub_n = 0;
  for (vertex_t vert_id = 0; vert_id < n; ++vert_id) {
    if (scc_id[vert_id] == scc_id_undecided) {
      sub_vertices[sub_n]           = vert_id;
      sub_vertices_inverse[vert_id] = sub_n;
      sub_n++;
    }
  }

  // fw
  index_t sub_fw_m = 0;
  sub_fw_beg[0]    = 0;
  for (vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
    auto const u   = sub_vertices[sub_u];
    auto const beg = fw_beg[u];
    auto const end = fw_beg[u + 1];
    for (auto it = beg; it < end; ++it) {
      auto const v = fw_csr[it];
      if (scc_id[v] == scc_id_undecided)
        sub_fw_csr[sub_fw_m++] = sub_vertices_inverse[v];
    }
    sub_fw_beg[sub_u + 1] = sub_fw_m;
  }

  // bw
  index_t sub_bw_m = 0;
  sub_bw_beg[0]    = 0;
  for (vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
    auto const u   = sub_vertices[sub_u];
    auto const beg = bw_beg[u];
    auto const end = bw_beg[u + 1];
    for (auto it = beg; it < end; ++it) {
      auto const v = bw_csr[it];
      if (scc_id[v] == scc_id_undecided)
        sub_bw_csr[sub_bw_m++] = sub_vertices_inverse[v];
    }
    sub_bw_beg[sub_u + 1] = sub_bw_m;
  }
}
