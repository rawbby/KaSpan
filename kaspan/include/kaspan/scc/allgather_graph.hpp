#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/scc/backward_complement.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan {

/**
 * @brief Collectively replicates a distributed CSR graph on all ranks.
 *
 * @details
 * Gathers the forward CSR head and edge arrays of a disjoint, ordered partition
 * across MPI_COMM_WORLD, converts concatenated local heads to a global head,
 * gathers forward edges accordingly, and builds the backward CSR locally.
 *
 * Memory/lifetime:
 * - result.fw_head, result.fw_csr, result.bw_head, result.bw_csr point into result.storage.
 * - Sizes: fw_head/bw_head = n + 1; fw_csr/bw_csr = m.
 * - Pointers remain valid as long as result.storage is alive.
 *
 * Invariants:
 * - result.n = n (global number of vertices).
 * - result.m = m (global number of edges).
 * - CSR layout: fw_head[u]..fw_head[u+1]-1 indexes neighbors in fw_csr (analogous for bw_*).
 *
 * @tparam Part Graph partition type satisfying world_part_concept and Part::ordered.
 * @param graph_part Distributed graph partition (disjoint global ownership).
 * @return local_graph containing replicated forward and backward CSR on every rank.
 *
 * @pre Collective over MPI_COMM_WORLD; all ranks must call.
 * @pre Part::ordered implies contiguous global vertex ranges; each global vertex is owned by exactly one rank.
 * @pre Uses MPI-4 “_c” collectives: counts are MPI_Count; displacements are byte offsets (MPI_Aint).
 * @pre index_t and vertex_t must map to supported MPI basic types.
 *
 * @note Handles ranks with local_n == 0 and empty graphs.
 *
 * @par Complexity
 * - Local work: O(n + m) (offset fix and backward construction).
 * - Communication: O(n + m) (all-gather of head and edges).
 */
template<part_view_concept Part>
  requires(Part::ordered)
auto
allgather_graph(
  index_t               m,
  graph_part_view<Part> gpv) -> bidi_graph
{
  auto const n       = gpv.part.n();
  auto const local_n = gpv.part.local_n();

  auto bg = bidi_graph{ n, m };

  // buffer counts and displs for multiple usages (cb guards lifetime)
  auto [cb, counts, displs] = mpi_basic::counts_and_displs();

  // 1) Allgather fw_head
  {
    auto const root_part = gpv.part.world_part_of(0);
    DEBUG_ASSERT_EQ(0, root_part.begin());
    counts[0] = integral_cast<MPI_Count>(root_part.end() + 1);
    displs[0] = integral_cast<MPI_Aint>(0);

    for (i32 rank = 1; rank < mpi_basic::world_size; ++rank) {
      auto const rank_part = gpv.part.world_part_of(rank);
      counts[rank]         = integral_cast<MPI_Count>(rank_part.end() - rank_part.begin());
      displs[rank]         = integral_cast<MPI_Aint>(rank_part.begin() + 1);
    }

    auto const* send_buffer = gpv.head + (mpi_basic::world_root ? 0 : 1);
    auto const  send_count  = local_n + mpi_basic::world_root;
    mpi_basic::allgatherv<index_t>(send_buffer, send_count, bg.fw.head, counts, displs);
  }

  // Per-rank offset fix: convert concatenated local heads to global head
  // and compute CSR recv counts/displs on the fly (reusing arrays).
  {
    // rank 0: edges count = fw_head[end0], displ = 0
    auto vertex_it = integral_cast<vertex_t>(counts[0]); // start at first vertex of rank 1
    auto offset    = bg.fw.head[vertex_it - 1];          // head at boundary (rank 0 end)
    counts[0]      = integral_cast<MPI_Count>(offset);   // edge elements of rank 0
    displs[0]      = integral_cast<MPI_Aint>(0);

    for (i32 rank = 1; rank < mpi_basic::world_size; ++rank) {
      for (MPI_Count k = 0; k < counts[rank]; ++k) {
        bg.fw.head[vertex_it++] += offset; // shift to global
      }
      auto const last = bg.fw.head[vertex_it - 1]; // global head at end of rank
      displs[rank]    = integral_cast<MPI_Aint>(offset);
      counts[rank]    = integral_cast<MPI_Count>(last - offset); // elements
      offset          = last;                                    // next rank base
    }
  }

  // 2) Gather forward CSR edges (reuse counts/displs computed above).
  mpi_basic::allgatherv<vertex_t>(gpv.csr, integral_cast<MPI_Count>(gpv.local_m), bg.fw.csr, counts, displs);

  // 3) Build backward graph locally from forward graph (no communication).
  backward_complement_graph(bg.view());

  return bg;
}

} // namespace kaspan
