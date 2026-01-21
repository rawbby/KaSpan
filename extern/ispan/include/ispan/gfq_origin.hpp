#pragma once

#include <ispan/util.hpp>

static void
gfq_origin(
  kaspan::vertex_t const  n,
  kaspan::vertex_t const* scc_id,
  kaspan::index_t const*  fw_head,
  kaspan::vertex_t const* fw_csr,
  kaspan::index_t const*  bw_head,
  kaspan::vertex_t const* bw_csr,

  kaspan::vertex_t& sub_n,
  kaspan::vertex_t* sub_vertices,
  kaspan::vertex_t* sub_vertices_inverse,
  kaspan::index_t*  sub_fw_head,
  kaspan::vertex_t* sub_fw_csr,
  kaspan::index_t*  sub_bw_head,
  kaspan::vertex_t* sub_bw_csr)
{
  sub_n = 0;
  for (kaspan::vertex_t vert_id = 0; vert_id < n; ++vert_id) {
    if (scc_id[vert_id] == kaspan::scc_id_undecided) {
      sub_vertices[sub_n]           = vert_id;
      sub_vertices_inverse[vert_id] = sub_n;
      sub_n++;
    }
  }

  // fw
  kaspan::index_t sub_fw_m = 0;
  sub_fw_head[0]           = 0;
  for (kaspan::vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
    auto const u   = sub_vertices[sub_u];
    auto const beg = fw_head[u];
    auto const end = fw_head[u + 1];
    for (auto it = beg; it < end; ++it) {
      auto const v = fw_csr[it];
      if (scc_id[v] == kaspan::scc_id_undecided) sub_fw_csr[sub_fw_m++] = sub_vertices_inverse[v];
    }
    sub_fw_head[sub_u + 1] = sub_fw_m;
  }

  // bw
  kaspan::index_t sub_bw_m = 0;
  sub_bw_head[0]           = 0;
  for (kaspan::vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
    auto const u   = sub_vertices[sub_u];
    auto const beg = bw_head[u];
    auto const end = bw_head[u + 1];
    for (auto it = beg; it < end; ++it) {
      auto const v = bw_csr[it];
      if (scc_id[v] == kaspan::scc_id_undecided) sub_bw_csr[sub_bw_m++] = sub_vertices_inverse[v];
    }
    sub_bw_head[sub_u + 1] = sub_bw_m;
  }
}
