#pragma once

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <scc/allgather_sub_graph.hpp>
#include <scc/backward_search.hpp>
#include <scc/base.hpp>
#include <scc/ecl_scc_step.hpp>
#include <scc/forward_search.hpp>
#include <scc/pivot_selection.hpp>
#include <scc/trim_1.hpp>
#include <scc/trim_tarjan.hpp>

#include <algorithm>
#include <cstdio>

template<WorldPartConcept Part>
void
scc(Part const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(part, bw_head, bw_csr);
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n;
  auto const local_n = part.local_n();

  vertex_t local_decided = 0;

  KASPAN_STATISTIC_PUSH("trim_1");
  // notice: trim_1_first has a side effect by initializing scc_id with scc_id undecided
  // if trim_1_first is removed one has to initialize scc_id with scc_id_undecided manually!
  auto const [trim_1_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
  KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(trim_1_decided, MPI_SUM));
  local_decided += trim_1_decided;
  
  if (mpi_world_rank == 0 && trim_1_decided > 0) {
    std::printf("[Rank %d] After trim_1: decided %d vertices\n", mpi_world_rank, trim_1_decided);
    for (vertex_t k = 0; k < local_n; ++k) {
      if (scc_id[k] != scc_id_undecided) {
        std::printf("  scc_id[%d] (u=%d) = %d\n", k, part.to_global(k), scc_id[k]);
      }
    }
  }
  
  KASPAN_STATISTIC_POP();

  // fallback to tarjan on single rank
  if (mpi_world_size == 1) {
    tarjan(part, fw_head, fw_csr, [=](auto const* cbeg, auto const* cend) {
      auto const id = *std::min_element(cbeg, cend);
      std::for_each(cbeg, cend, [=](auto const k) {
        scc_id[k] = id;
      });
    },
           SCC_ID_UNDECIDED_FILTER(local_n, scc_id),
           local_decided);
    return;
  }

  auto const decided_threshold = n - (2 * n / mpi_world_size); // as we only gather fw graph we can only reduce to 2 * local_n
  DEBUG_ASSERT_GE(decided_threshold, 0);

  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("forward_backward_search");

    auto frontier   = vertex_frontier::create();
    auto first_root = pivot_selection(max);
    DEBUG_ASSERT_NE(first_root, scc_id_undecided);
    auto       bitvector  = make_bits_clean(local_n);
    auto const first_id   = forward_search(part, fw_head, fw_csr, frontier, scc_id, bitvector, first_root);
    auto const fb_decided = backward_search(part, bw_head, bw_csr, frontier, scc_id, bitvector, first_root, first_id);
    local_decided += fb_decided;

    if (mpi_world_rank == 0 && fb_decided > 0) {
      std::printf("[Rank %d] After forward_backward_search: decided %d vertices (first_root=%d, first_id=%d)\n", 
                  mpi_world_rank, fb_decided, first_root, first_id);
      for (vertex_t k = 0; k < local_n; ++k) {
        if (scc_id[k] != scc_id_undecided) {
          std::printf("  scc_id[%d] (u=%d) = %d\n", k, part.to_global(k), scc_id[k]);
        }
      }
    }

    KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(fb_decided, MPI_SUM));
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  }

  if (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("ecl");

    auto fw_label     = make_array<vertex_t>(local_n);
    auto bw_label     = make_array<vertex_t>(local_n);
    auto active_array = make_array<vertex_t>(local_n - local_decided);
    auto active       = make_bits_clean(local_n);
    auto changed      = make_bits_clean(local_n);
    auto frontier     = edge_frontier::create();

    vertex_t ecl_decided_before = local_decided;
    do {
      ecl_scc_init_lable(part, fw_label, bw_label);
      local_decided += ecl_scc_step(
        part, fw_head, fw_csr, bw_head, bw_csr, scc_id, fw_label, bw_label, active_array, active, changed, frontier, local_decided);
      // maybe: redistribute graph - sort vertices by ecl label and run trim tarjan (as there is now a lot locality)
    } while (mpi_basic_allreduce_single(local_decided, MPI_SUM) < decided_threshold);

    vertex_t ecl_decided = local_decided - ecl_decided_before;
    if (mpi_world_rank == 0 && ecl_decided > 0) {
      std::printf("[Rank %d] After ecl: decided %d vertices\n", mpi_world_rank, ecl_decided);
      for (vertex_t k = 0; k < local_n; ++k) {
        if (scc_id[k] != scc_id_undecided) {
          std::printf("  scc_id[%d] (u=%d) = %d\n", k, part.to_global(k), scc_id[k]);
        }
      }
    }

    KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(local_n - local_decided, MPI_SUM));
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  }

  // local_decided += trim_tarjan(part, fw_head, fw_csr, bw_head, bw_csr, scc_id, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));

  {
    KASPAN_STATISTIC_SCOPE("residual");

    // Log undecided vertices before residual phase
    if (mpi_world_rank == 0) {
      std::printf("[Rank %d] Undecided vertices before residual: ", mpi_world_rank);
      for (vertex_t k = 0; k < local_n; ++k) {
        if (scc_id[k] == scc_id_undecided) {
          std::printf("%d ", part.to_global(k));
        }
      }
      std::printf("(count=%d)\n", local_n - local_decided);
    }

    auto [TMP(), sub_ids_inverse, TMP(), sub_n, sub_m, sub_head, sub_csr] =
      allgather_fw_sub_graph(part, local_n - local_decided, fw_head, fw_csr, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    DEBUG_ASSERT_VALID_GRAPH(sub_n, sub_m, sub_head, sub_csr);

    // Log sub-graph structure
    if (mpi_world_rank == 0) {
      std::printf("[Rank %d] Sub-graph: n=%d, m=%d\n", mpi_world_rank, sub_n, sub_m);
      std::printf("[Rank %d] sub_ids_inverse: [", mpi_world_rank);
      for (vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
        if (sub_u > 0) std::printf(", ");
        std::printf("%d", sub_ids_inverse[sub_u]);
      }
      std::printf("]\n");
      
      std::printf("[Rank %d] Sub-graph edges:\n", mpi_world_rank);
      for (vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
        auto const u = sub_ids_inverse[sub_u];
        std::printf("  %d (global %d): ", sub_u, u);
        for (index_t it = sub_head[sub_u]; it < sub_head[sub_u + 1]; ++it) {
          auto const sub_v = sub_csr[it];
          auto const v = sub_ids_inverse[sub_v];
          std::printf("%d->%d ", u, v);
        }
        std::printf("\n");
      }
    }

    // Verify sub_ids_inverse is correctly populated
    for (vertex_t sub_u = 0; sub_u < sub_n; ++sub_u) {
      auto const u = sub_ids_inverse[sub_u];
      DEBUG_ASSERT_GE(u, 0, "sub_u={}, u={}", sub_u, u);
      DEBUG_ASSERT_LT(u, n, "sub_u={}, u={}, n={}", sub_u, u, n);
      
      // CRITICAL CHECK: Verify that this vertex is actually undecided locally
      if (part.has_local(u)) {
        auto const k = part.to_local(u);
        DEBUG_ASSERT_EQ(
          scc_id[k], 
          scc_id_undecided, 
          "BUG: Vertex in sub-graph is already decided! sub_u={}, u={}, k={}, scc_id[k]={}, expected=undecided. "
          "This means a vertex that was already assigned an SCC ID is being included in the residual sub-graph, "
          "which will cause Tarjan to incorrectly merge distinct SCCs.",
          sub_u, u, k, scc_id[k]);
      }
    }

    if (sub_n) {
      vertex_t scc_count = 0;
      tarjan(sub_n, sub_head, sub_csr, [&](auto const* beg, auto const* end) {
        DEBUG_ASSERT_NE(beg, nullptr);
        DEBUG_ASSERT_NE(end, nullptr);
        DEBUG_ASSERT_LT(beg, end, "Empty SCC component");
        
        auto const min_sub_u = *std::min_element(beg, end);
        DEBUG_ASSERT_GE(min_sub_u, 0, "min_sub_u={}", min_sub_u);
        DEBUG_ASSERT_LT(min_sub_u, sub_n, "min_sub_u={}, sub_n={}", min_sub_u, sub_n);
        
        auto const min_u = sub_ids_inverse[min_sub_u];
        DEBUG_ASSERT_GE(min_u, 0, "min_sub_u={}, min_u={}", min_sub_u, min_u);
        DEBUG_ASSERT_LT(min_u, n, "min_sub_u={}, min_u={}, n={}", min_sub_u, min_u, n);
        
        // Log SCC details
        std::vector<vertex_t> scc_vertices;
        for (auto sub_u : std::span{ beg, end }) {
          scc_vertices.push_back(sub_ids_inverse[sub_u]);
        }
        std::sort(scc_vertices.begin(), scc_vertices.end());
        
        if (mpi_world_rank == 0) {
          std::printf("[Rank %d] SCC #%d: min_u=%d, vertices=[", mpi_world_rank, scc_count, min_u);
          for (size_t i = 0; i < scc_vertices.size(); ++i) {
            if (i > 0) std::printf(", ");
            std::printf("%d", scc_vertices[i]);
          }
          std::printf("]\n");
        }
        scc_count++;
        
        for (auto sub_u : std::span{ beg, end }) {
          DEBUG_ASSERT_GE(sub_u, 0, "sub_u={}", sub_u);
          DEBUG_ASSERT_LT(sub_u, sub_n, "sub_u={}, sub_n={}", sub_u, sub_n);
          
          auto const u = sub_ids_inverse[sub_u];
          DEBUG_ASSERT_GE(u, 0, "sub_u={}, u={}", sub_u, u);
          DEBUG_ASSERT_LT(u, n, "sub_u={}, u={}, n={}", sub_u, u, n);
          
          if (part.has_local(u)) {
            auto const k = part.to_local(u);
            DEBUG_ASSERT_EQ(scc_id[k], scc_id_undecided, "Vertex already decided: k={}, u={}, scc_id[k]={}", k, u, scc_id[k]);
            scc_id[k] = min_u;
            if (mpi_world_rank == 0) {
              std::printf("[Rank %d] Assigning: scc_id[%d] (u=%d) = %d\n", mpi_world_rank, k, u, min_u);
            }
          }
        }
      });
    }

    KASPAN_STATISTIC_ADD("n", sub_n);
    KASPAN_STATISTIC_ADD("m", sub_m);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
    KASPAN_STATISTIC_ADD("decided_count", mpi_basic_allreduce_single(local_n - local_decided, MPI_SUM));
  }
}
