#pragma once

#include <ispan/util.hpp>
#include <util/scope_guard.hpp>

#include <algorithm>

inline void
residual_scc(index_t const* sub_wcc_id, index_t* sub_scc_id, index_t const* sub_fw_head, index_t const* sub_bw_head, vertex_t const* sub_fw_csr, vertex_t const* sub_bw_csr, vertex_t* sub_fw_sa, index_t world_rank, index_t world_size, vertex_t sub_n, vertex_t const* sub_wcc_fq, vertex_t sub_wcc_fq_size, vertex_t const* sub_vertices)
{
  KASPAN_STATISTIC_SCOPE("residual_scc");
  size_t decided_count = 0;

  // prepare the local slice this rank calculates

  index_t const step         = (sub_wcc_fq_size + world_size - 1) / world_size;
  index_t const rank_wcc_beg = world_rank * step;
  index_t const rank_wcc_end = rank_wcc_beg + step;

  auto* q = new index_t[sub_n]{};
  SCOPE_GUARD(delete[] q);

  index_t head = 0;
  index_t tail = 0;

  for (vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
    if (sub_scc_id[sub_u] == scc_id_undecided) {

      auto const cur_wcc = sub_wcc_id[sub_u];
      bool       in_wcc  = false;
      for (vertex_t i = rank_wcc_beg; i < rank_wcc_end; ++i) {
        if (sub_wcc_fq[i] == cur_wcc) {
          in_wcc = true;
          break;
        }
      }

      if (in_wcc) {
        // start a fw bw search from sub_u as root
        auto const id     = sub_vertices[sub_u];
        sub_scc_id[sub_u] = id;
        ++decided_count;
        sub_fw_sa[sub_u] = sub_u;

        // fw search
        q[tail++] = sub_u;
        if (tail == sub_n)
          tail = 0;
        while (head != tail) {
          auto const temp_v = q[head++];
          if (head == sub_n)
            head = 0;

          auto       beg = sub_fw_head[temp_v];
          auto const end = sub_fw_head[temp_v + 1];
          for (; beg < end; ++beg) {
            auto const sub_v = sub_fw_csr[beg];

            if (sub_scc_id[sub_v] != scc_id_undecided)
              continue;

            if (sub_fw_sa[sub_v] != sub_u) {
              // if sub_v was not yet reached
              // add sub_v to queue and mark sub_v reached

              q[tail++] = sub_v;
              if (tail == sub_n)
                tail = 0;
              sub_fw_sa[sub_v] = sub_u;
            }
          }
        }

        // bw search
        q[tail++] = sub_u;
        if (tail == sub_n)
          tail = 0;
        while (head != tail) {
          auto const temp_v = q[head++];
          if (head == sub_n)
            head = 0;

          auto       beg = sub_bw_head[temp_v];
          auto const end = sub_bw_head[temp_v + 1];
          for (; beg < end; ++beg) {
            auto const sub_v = sub_bw_csr[beg];

            if (sub_scc_id[sub_v] == scc_id_undecided and sub_fw_sa[sub_v] == sub_u) {
              // if sub_v is undecided and reached in fw search
              // add sub_v to queue and set scc id of sub_v

              q[tail++] = sub_v;
              if (tail == sub_n)
                tail = 0;
              sub_scc_id[sub_v] = id;
              ++decided_count;
            }
          }
        }
      }
    }
  }

  KASPAN_STATISTIC_ADD("decided_count", decided_count);
}
