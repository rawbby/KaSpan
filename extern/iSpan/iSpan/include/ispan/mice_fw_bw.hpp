#pragma once

#include <ispan/util.hpp>

inline void
mice_fw_bw(int const* wcc_color, index_t* scc_id, index_t const* sub_fw_beg, index_t const* sub_bw_beg, vertex_t const* sub_fw_csr, vertex_t const* sub_bw_csr, vertex_t* fw_sa, index_t world_rank, index_t world_size, vertex_t sub_v_count, vertex_t const* wcc_fq, vertex_t wcc_fq_size)
{
  auto const step    = wcc_fq_size / world_size;
  auto const wcc_beg = std::min<index_t>(world_rank * step, wcc_fq_size);
  auto const wcc_end = std::min<index_t>(wcc_beg + step, wcc_fq_size);

  auto* q = new index_t[sub_v_count]{};
  SCOPE_GUARD(delete[] q);

  index_t head = 0;
  index_t tail = 0;
  for (vertex_t v = 0; v < sub_v_count; ++v) {
    if (scc_id[v] == -1) {

      vertex_t const cur_wcc = wcc_color[v];
      bool           in_wcc  = false;
      for (vertex_t i = wcc_beg; i < wcc_end; ++i) {
        if (wcc_fq[i] == cur_wcc) {
          in_wcc = true;
          break;
        }
      }

      if (in_wcc) {
        fw_sa[v]  = v;
        q[tail++] = v;
        if (tail == sub_v_count)
          tail = 0;
        while (head != tail) {
          auto const temp_v = q[head++];
          if (head == sub_v_count)
            head = 0;
          auto       my_beg = sub_fw_beg[temp_v];
          auto const my_end = sub_fw_beg[temp_v + 1];
          for (; my_beg < my_end; ++my_beg) {
            auto const w = sub_fw_csr[my_beg];
            if (fw_sa[w] != v) {
              q[tail++] = w;
              if (tail == sub_v_count)
                tail = 0;
              fw_sa[w] = v;
            }
          }
        }
        scc_id[v] = v;
        q[tail++] = v;
        if (tail == sub_v_count)
          tail = 0;
        while (head != tail) {
          auto const temp_v = q[head++];
          if (head == sub_v_count)
            head = 0;
          auto       my_beg = sub_bw_beg[temp_v];
          auto const my_end = sub_bw_beg[temp_v + 1];
          for (; my_beg < my_end; ++my_beg) {
            auto const w = sub_bw_csr[my_beg];
            if (scc_id[w] == -1 && fw_sa[w] == v) {
              q[tail++] = w;
              if (tail == sub_v_count)
                tail = 0;
              scc_id[w] = v;
            }
          }
        }
      }
    }
  }
}
