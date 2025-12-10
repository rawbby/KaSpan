#pragma once

#include <memory/buffer.hpp>
#include <memory/stack_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <scc/part.hpp>

/**
 * @brief Result of an all-gathered induced subgraph.
 *
 * @details
 * Holds a replicated CSR representation of the induced subgraph on every rank,
 * plus the new-to-old vertex mapping.
 *
 * Memory/lifetime:
 * - ids_inverse points into ids_inverse_buffer (size n).
 * - fw_head, fw_csr, bw_head, bw_csr point into buffer (sizes: fw_head/bw_head = n+1, fw_csr/bw_csr = m).
 * - Pointers remain valid as long as the corresponding Buffer members are alive.
 *
 * Invariants:
 * - n = number of vertices in the subgraph.
 * - m = number of edges in the subgraph (for each orientation).
 * - ids_inverse[new_id] = global vertex id.
 * - CSR layout: fw_head[u]..fw_head[u+1]-1 indexes neighbors in fw_csr (analogous for bw_*).
 */
struct AllGatherSubGraphResult
{
  Buffer    ids_inverse_buffer;
  vertex_t* ids_inverse;

  Buffer    buffer;
  vertex_t  n;
  index_t   m;
  index_t*  fw_head;
  vertex_t* fw_csr;
  index_t*  bw_head;
  vertex_t* bw_csr;
};

/**
 * @brief Collectively builds and replicates the induced subgraph defined by a vertex filter.
 *
 * @tparam Part Graph partition type; must satisfy Part::ordered.
 * @tparam Fn   Predicate type; callable as bool(vertex_t local_id).
 *
 * @param part Distributed graph partition (disjoint global ownership).
 * @param in_sub_graph Predicate on local vertex ids; true selects the vertex.
 *
 * @return AllGatherSubGraphResult containing:
 *         - ids_inverse: new->old (global) vertex mapping,
 *         - fw_ / bw_: CSR of the induced subgraph, replicated on all ranks.
 *
 * @pre Collective over MPI_COMM_WORLD; all ranks must call.
 * @pre Each global vertex is owned by exactly one rank (disjoint partition).
 * @pre Uses MPI-4 “_c” collectives: counts are MPI_Count; displacements are byte offsets (MPI_Aint).
 *
 * @note New vertex ids [0..n) follow the global order produced by the all-gather of selected global ids.
 *
 * @par Complexity
 *
 * Linear.
 *
 * 1. select local sub vertices (inverse mapping: new->old). Local work: O(local_n)
 * 2. replicate sub vertices. Communication: O(sub_n)
 * 3. compute inverse inverse mapping (old->new). Local work: O(sub_n)
 * 4. compute out degrees and forward CSR per local sub vertex. Local work: O(local_sub_fw_m)
 * 5. replicate out degrees and forward CSR. Communication: O(sub_m + sub_n)
 * 6. convert out degrees into head. Local work: O(sub_n)
 * 7. compute in degrees and backward CSR per local sub vertex. Local work: O(local_sub_bw_m)
 * 8. replicate in degrees and backward CSR. Communication: O(sub_m + sub_n)
 * 9. convert in degrees into head. Local work: O(sub_n)
 *
 * Total local work: O(local_n + local_fw_m + local_bw_m + sub_n).
 *
 * Total communication: O(sub_n + sub_m).
 */
template<class Part, class Fn>
  requires(Part::ordered and std::convertible_to<std::invoke_result_t<Fn, vertex_t>, bool>)
auto
allgather_sub_graph(
  Part const&     part,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  Fn&&            in_sub_graph) -> AllGatherSubGraphResult
{
  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(part, bw_head, bw_csr);

  auto const local_n    = part.local_n();
  auto const local_fw_m = fw_head[local_n];
  auto const local_bw_m = bw_head[local_n];

  Buffer temp_buffer{
    page_ceil<vertex_t>(local_n),
    page_ceil<vertex_t>(std::max(local_fw_m, local_bw_m)),
    page_ceil<index_t>(local_n + 1)
  };
  void* temp_memory = temp_buffer.data();

  auto local_ids_inverse = StackAccessor<vertex_t>::borrow(temp_memory, local_n);
  for (vertex_t k = 0; k < local_n; ++k) {
    if (static_cast<bool>(in_sub_graph(k))) {
      local_ids_inverse.push(part.to_global(k));
    }
  }

  auto const local_sub_n    = local_ids_inverse.size();
  auto [cb, counts, displs] = mpi_basic_counts_and_displs();

  // gather the ids_inverse array
  AllGatherSubGraphResult sub;
  mpi_basic_allgatherv_counts(local_sub_n, counts);
  sub.n                  = static_cast<vertex_t>(mpi_basic_displs(counts, displs));
  sub.ids_inverse_buffer = Buffer{ sub.n * sizeof(vertex_t) };
  sub.ids_inverse        = static_cast<vertex_t*>(sub.ids_inverse_buffer.data());
  mpi_basic_allgatherv<vertex_t>(local_ids_inverse.data(), local_sub_n, sub.ids_inverse, counts, displs);

  // compute sub_ids mapping to check
  // containment + mapping in reasonable space and time
  std::unordered_map<vertex_t, vertex_t> sub_ids;
  sub_ids.reserve(sub.n);
  for (vertex_t new_id = 0; new_id < sub.n; ++new_id) {
    sub_ids.emplace(sub.ids_inverse[new_id], new_id);
  }

  // graph memory (iterator), updated (advanced) on borrow calls
  void* graph_memory = nullptr;

  // communicate forward graph
  {
    void* scope_memory         = temp_memory;
    auto  local_sub_fw_csr     = StackAccessor<vertex_t>::borrow(scope_memory, local_fw_m);
    auto  local_sub_fw_degrees = StackAccessor<index_t>::borrow(scope_memory, local_sub_n + mpi_world_root);
    if (mpi_world_root) {
      local_sub_fw_degrees.push(0);
    }

    for (vertex_t i = 0; i < local_sub_n; ++i) { // for each vertex in part and in sub graph
      auto const u   = local_ids_inverse[i];
      auto const k   = part.to_local(u);
      auto const beg = fw_head[k];
      auto const end = fw_head[k + 1];

      index_t deg = 0;
      for (index_t it = beg; it < end; ++it) {
        auto const v = fw_csr[it];
        // v is in sub graph if contained by sub_ids
        if (auto const jt = sub_ids.find(v); jt != sub_ids.end()) {
          local_sub_fw_csr.push(jt->second);
          ++deg;
        }
      }
      local_sub_fw_degrees.push(deg);
    }

    // allocate graph buffer and gather forward csr
    {
      mpi_basic_allgatherv_counts(local_sub_fw_csr.size(), counts);
      auto const sub_m = mpi_basic_displs(counts, displs);

      // allocate sub graph memory in one buffer as now sub_n and sub_m are known
      sub.buffer = Buffer(
        2 * page_ceil<index_t>(sub.n + 1),
        2 * page_ceil<vertex_t>(sub_m));

      graph_memory = sub.buffer.data();
      sub.m        = static_cast<index_t>(sub_m);

      // set forward memory and gather the final csr
      sub.fw_csr = borrow<vertex_t>(graph_memory, sub_m);
      mpi_basic_allgatherv<vertex_t>(local_sub_fw_csr.data(), local_sub_fw_csr.size(), sub.fw_csr, counts, displs);
    }

    // gather global degrees in head and convert to head afterwards
    {
      auto const send_count = local_sub_n + mpi_world_root;
      mpi_basic_allgatherv_counts(send_count, counts);
      auto const recv_count = mpi_basic_displs(counts, displs);
      DEBUG_ASSERT_EQ(sub.n + 1, recv_count);
      sub.fw_head = borrow<index_t>(graph_memory, recv_count);
      mpi_basic_allgatherv<index_t>(local_sub_fw_degrees.data(), send_count, sub.fw_head, counts, displs);
    }
    for (vertex_t u = 0; u < sub.n; ++u) {
      sub.fw_head[u + 1] = sub.fw_head[u] + sub.fw_head[u + 1];
    }
  }

  // communicate backward graph
  {
    void* scope_memory         = temp_memory;
    auto  local_sub_bw_csr     = StackAccessor<vertex_t>::borrow(scope_memory, local_bw_m);
    auto  local_sub_bw_degrees = StackAccessor<index_t>::borrow(scope_memory, local_sub_n + mpi_world_root);
    if (mpi_world_root) {
      local_sub_bw_degrees.push(0);
    }

    for (vertex_t i = 0; i < local_sub_n; ++i) { // for each vertex in part and in sub graph
      auto const u   = local_ids_inverse[i];
      auto const k   = part.to_local(u);
      auto const beg = bw_head[k];
      auto const end = bw_head[k + 1];

      index_t deg = 0;
      for (index_t it = beg; it < end; ++it) {
        auto const v = bw_csr[it];
        // v is in sub graph if contained by sub_ids
        if (auto const jt = sub_ids.find(v); jt != sub_ids.end()) {
          local_sub_bw_csr.push(jt->second);
          ++deg;
        }
      }
      local_sub_bw_degrees.push(deg);
    }

    // gather backward csr
    {
      mpi_basic_allgatherv_counts(local_sub_bw_csr.size(), counts);
      auto const recv_count = mpi_basic_displs(counts, displs);
      DEBUG_ASSERT_EQ(sub.m, recv_count);
      sub.bw_csr = borrow<vertex_t>(graph_memory, recv_count);
      mpi_basic_allgatherv<vertex_t>(local_sub_bw_csr.data(), local_sub_bw_csr.size(), sub.bw_csr, counts, displs);
    }

    // gather global degrees in head and convert to head afterwards
    {
      auto const send_count = local_sub_n + mpi_world_root;
      mpi_basic_allgatherv_counts(send_count, counts);
      auto const recv_count = mpi_basic_displs(counts, displs);
      DEBUG_ASSERT_EQ(sub.n + 1, recv_count);
      sub.bw_head = borrow<index_t>(graph_memory, recv_count);
      mpi_basic_allgatherv<index_t>(local_sub_bw_degrees.data(), send_count, sub.bw_head, counts, displs);
    }
    for (vertex_t u = 0; u < sub.n; ++u) {
      sub.bw_head[u + 1] = sub.bw_head[u] + sub.bw_head[u + 1];
    }
  }

  DEBUG_ASSERT_VALID_GRAPH(sub.n, sub.fw_head[sub.n], sub.fw_head, sub.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(sub.n, sub.bw_head[sub.n], sub.bw_head, sub.bw_csr);
  return sub;
}
