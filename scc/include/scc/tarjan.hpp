#pragma once

#include <scc/Common.hpp>

#include <buffer/ArrayAccessor.hpp>
#include <buffer/BitAccessor.hpp>
#include <buffer/StackAccessor.hpp>
#include <debug/Statistic.hpp>
#include <scc/Graph.hpp>

/**
 * @param[in] part partition of the graph to work on
 * @param[in] head head buffer of size local_n+1 pointing into csr entries
 * @param[in] csr compressed sparse row of the local partition of the graph
 * @param[out] scc_id map vertex to strongly connected component id
 * @param[in] memory temp memory of:
 *   - 3 * page_ceil(local_n * sizeof(vertex_t))                 // index, low, st
 *   - page_ceil(local_n * (sizeof(vertex_t) + sizeof(index_t))) // dfs stack
 *   - page_ceil(ceil(local_n/64) * sizeof(u64))                 // on_stack bitset
 * @return number of strongly connected components
 */
template<WorldPartConcept Part>
vertex_t
tarjan(Part const& part,
       index_t const*  head,
       vertex_t const*  csr,
       vertex_t*  scc_id,
       void*  memory) noexcept
{
  KASPAN_STATISTIC_SCOPE("tarjan");
  constexpr auto index_undecided = scc_id_undecided;
  struct Frame
  {
    vertex_t local_u;
    index_t  it;
  };

  vertex_t const local_n      = part.size();
  vertex_t       index_count  = 0;
  vertex_t       scc_id_count = 0;

  auto index    = ArrayAccessor<vertex_t>::borrow_filled(memory, index_undecided, local_n);
  auto low      = ArrayAccessor<vertex_t>::borrow_clean(memory, local_n);
  auto on_stack = BitAccessor::borrow_clean(memory, local_n);
  auto st       = StackAccessor<vertex_t>::borrow(memory, local_n);
  auto dfs      = StackAccessor<Frame>::borrow(memory, local_n);

  for (vertex_t local_root = 0; local_root < local_n; ++local_root) {
    if (index[local_root] != index_undecided)
      continue;

    index[local_root] = low[local_root] = index_count++;
    st.push(local_root);
    on_stack.set(local_root);
    dfs.push({ local_root, head[local_root] });

    while (not dfs.empty()) {

      auto& [local_u, it] = dfs.back();
      if (it < head[local_u + 1]) {
        auto const v = csr[it++];

        if (part.contains(v)) { // ignore non local edges
          auto const local_v = part.rank(v);
          auto const v_index = index[local_v];

          if (v_index == index_undecided) {
            index[local_v] = low[local_v] = index_count++;
            st.push(local_v);
            on_stack.set(local_v);
            dfs.push({ local_v, head[local_v] });
          }

          else if (on_stack.get(local_v) and v_index < low[local_u]) {
            low[local_u] = v_index;
          }
        }

        continue;
      }

      if (low[local_u] == index[local_u]) {
        while (true) {
          auto const local_v = st.back();
          st.pop();
          on_stack.unset(local_v);
          scc_id[local_v] = scc_id_count;
          if (local_v == local_u)
            break;
        }
        ++scc_id_count;
      }

      dfs.pop();
      if (not dfs.empty()) {
        auto const local_p = dfs.back().local_u;
        low[local_p]       = std::min(low[local_p], low[local_u]);
      }
    }
  }

  return scc_id_count;
}

/**
 * @param[in] n number of vertices in the graph
 * @param[in] head head buffer of size n+1 pointing into csr entries
 * @param[in] csr compressed sparse row of the graph
 * @param[out] scc_id map vertex to strongly connected component id
 * @param[in] memory temp memory of:
 *   - 3 * page_ceil(n * sizeof(vertex_t))                 // index, low, st
 *   - page_ceil(n * (sizeof(vertex_t) + sizeof(index_t))) // dfs stack
 *   - page_ceil(ceil(n/64) * sizeof(u64))                 // on_stack bitset
 * @return number of strongly connected components
 */
inline vertex_t
tarjan(vertex_t const n,
       index_t const*  head,
       vertex_t const*  csr,
       vertex_t*  scc_id,
       void*  memory) noexcept
{
  return tarjan(SingleWorldPart{ n }, head, csr, scc_id, memory);
}
