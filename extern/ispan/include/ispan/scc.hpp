#pragma once
#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>

#include <ispan/fw_bw_span.hpp>
#include <ispan/get_scc_result.hpp>
#include <ispan/gfq_origin.hpp>
#include <ispan/pivot_selection.hpp>
#include <ispan/process_wcc.hpp>
#include <ispan/residual_scc.hpp>
#include <ispan/residual_wcc.hpp>
#include <ispan/trim_1_first.hpp>
#include <ispan/trim_1_normal.hpp>
#include <ispan/util.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mpi.h>
#include <unistd.h>
#include <vector>

inline void
scc(
  kaspan::vertex_t               n,
  kaspan::index_t                m,
  kaspan::index_t const*         fw_head,
  kaspan::vertex_t const*        fw_csr,
  kaspan::index_t const*         bw_head,
  kaspan::vertex_t const*        bw_csr,
  int                            alpha,
  std::vector<kaspan::vertex_t>* scc_id_out = nullptr)
{
  KASPAN_STATISTIC_SCOPE("scc");
  KASPAN_STATISTIC_PUSH("alloc");

  auto* front_comm = new kaspan::index_t[kaspan::mpi_basic::world_size]{};
  SCOPE_GUARD(delete[] front_comm);

  auto* work_comm = new int[kaspan::mpi_basic::world_size]{};
  SCOPE_GUARD(delete[] work_comm);

  kaspan::vertex_t s = n / 32;
  if (n % 32 != 0) s += 1;

  kaspan::vertex_t t = s / kaspan::mpi_basic::world_size;
  if (s % kaspan::mpi_basic::world_size != 0) t += 1;

  kaspan::vertex_t const step          = t * 32;
  kaspan::vertex_t const virtual_count = t * kaspan::mpi_basic::world_size * 32;
  kaspan::vertex_t const local_beg     = std::min<kaspan::index_t>(step * kaspan::mpi_basic::world_rank, n);
  kaspan::vertex_t const local_end     = std::min<kaspan::index_t>(step * (kaspan::mpi_basic::world_rank + 1), n);

  auto* sa_compress = new unsigned int[s]{};
  SCOPE_GUARD(delete[] sa_compress);

  auto* sub_vertices = new kaspan::vertex_t[virtual_count]{};
  SCOPE_GUARD(delete[] sub_vertices);

  auto* sub_wcc_fq = new kaspan::vertex_t[virtual_count]{};
  SCOPE_GUARD(delete[] sub_wcc_fq);

  auto* sub_vertices_inverse = new kaspan::vertex_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_vertices_inverse);

  auto* sub_fw_head = new kaspan::index_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_fw_head);

  auto* sub_fw_csr = new kaspan::vertex_t[m + 1]{};
  SCOPE_GUARD(delete[] sub_fw_csr);

  auto* sub_bw_head = new kaspan::index_t[n + 1]{};
  SCOPE_GUARD(delete[] sub_bw_head);

  auto* sub_bw_csr = new kaspan::vertex_t[m + 1]{};
  SCOPE_GUARD(delete[] sub_bw_csr);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
  depth_t* fw_sa = nullptr;
  ASSERT(posix_memalign((void**)&fw_sa, getpagesize(), sizeof(*fw_sa) * (virtual_count + 1)) == 0);
  SCOPE_GUARD(free(fw_sa));
  depth_t* bw_sa = nullptr;
  ASSERT(posix_memalign((void**)&bw_sa, getpagesize(), sizeof(*bw_sa) * (virtual_count + 1)) == 0);
  SCOPE_GUARD(free(bw_sa));
#pragma GCC diagnostic pop

  auto* fq_comm = new kaspan::vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] fq_comm);

  auto* scc_id = new kaspan::vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] scc_id);

  auto* sub_scc_id = new kaspan::vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] sub_scc_id);

  auto* sub_fw_sa = new kaspan::vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] sub_fw_sa);

  auto* sub_wcc_id = new kaspan::vertex_t[virtual_count + 1]{};
  SCOPE_GUARD(delete[] sub_wcc_id);

  for (int i = 0; i < virtual_count + 1; ++i) {
    fw_sa[i]      = depth_unset;
    bw_sa[i]      = depth_unset;
    scc_id[i]     = kaspan::scc_id_undecided;
    sub_scc_id[i] = kaspan::scc_id_undecided;
    sub_fw_sa[i]  = kaspan::scc_id_undecided;
  }

  KASPAN_STATISTIC_ADD("memory", kaspan::get_resident_set_bytes());
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("trim_1_first");
  size_t trim_1_decisions = trim_1_first(scc_id, fw_head, bw_head, local_beg, local_end, step);
  KASPAN_STATISTIC_ADD("decided_count", trim_1_decisions);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("forward_backward_search");
  auto const root = pivot_selection(scc_id, fw_head, bw_head, n);
  fw_span(scc_id, fw_head, bw_head, fw_csr, bw_csr, local_beg, local_end, fw_sa, front_comm, root, alpha, n, m, step, fq_comm, sa_compress);
  memset(sa_compress, 0, s * sizeof(*sa_compress));
  size_t bw_decisions = bw_span(scc_id, fw_head, bw_head, fw_csr, bw_csr, local_beg, local_end, fw_sa, bw_sa, front_comm, work_comm, root, alpha, n, m, step, fq_comm, sa_compress);
  KASPAN_STATISTIC_ADD("decided_count", bw_decisions);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("trim_1_normal");
  size_t trim_1_normal_decisions = 0;
  trim_1_normal_decisions += trim_1_normal(scc_id, fw_head, bw_head, fw_csr, bw_csr, local_beg, local_end);
  trim_1_normal_decisions += trim_1_normal(scc_id, fw_head, bw_head, fw_csr, bw_csr, local_beg, local_end);
  KASPAN_STATISTIC_ADD("decided_count", trim_1_normal_decisions);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("residual");
  // clang-format off
  MPI_Allreduce(
    MPI_IN_PLACE, scc_id, n, kaspan::mpi_vertex_t,
    MPI_MIN, MPI_COMM_WORLD);
  // clang-format on
  kaspan::vertex_t sub_n = 0;
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
    for (kaspan::index_t i = 0; i < sub_n; ++i)
      sub_wcc_id[i] = i;

    wcc(sub_wcc_id, sub_fw_head, sub_fw_csr, sub_bw_head, sub_bw_csr, sub_n);
    kaspan::vertex_t sub_wcc_fq_size = 0;
    process_wcc(sub_n, sub_wcc_fq, sub_wcc_id, sub_wcc_fq_size);

    residual_scc(sub_wcc_id, sub_scc_id, sub_fw_head, sub_bw_head, sub_fw_csr, sub_bw_csr, sub_fw_sa, sub_n, sub_wcc_fq, sub_wcc_fq_size, sub_vertices);

    MPI_Allreduce(MPI_IN_PLACE, sub_scc_id, sub_n, kaspan::mpi_vertex_t, MPI_MIN, MPI_COMM_WORLD);
    for (kaspan::index_t sub_u = 0; sub_u < sub_n; ++sub_u) {
      if (sub_scc_id[sub_u] != kaspan::scc_id_undecided) scc_id[sub_vertices[sub_u]] = sub_scc_id[sub_u];
    }
    KASPAN_STATISTIC_ADD("memory", kaspan::get_resident_set_bytes());
  }
  KASPAN_STATISTIC_ADD("decided_count", local_end - local_beg - trim_1_decisions - bw_decisions - trim_1_normal_decisions);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_SCOPE("post_processing");
  get_scc_result(scc_id, n);
  if (scc_id_out != nullptr) {
    scc_id_out->resize(n);
    std::memcpy(scc_id_out->data(), scc_id, n * sizeof(kaspan::vertex_t));
  }
}
