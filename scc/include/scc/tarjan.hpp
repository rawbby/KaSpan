#pragma once

#include "memory/accessor/bits.hpp"

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <iostream>
#include <memory/accessor/bits_accessor.hpp>
#include <memory/accessor/bits.hpp>
#include <memory/accessor/stack_accessor.hpp>
#include <memory/accessor/stack.hpp>
#include <memory/buffer.hpp>
#include <scc/graph.hpp>

constexpr auto
no_filter(vertex_t /* k */) -> bool
{
  return true;
}

/**
 * @param[in] part partition of the graph to work on
 * @param[in] head head buffer of size local_n+1 pointing into csr entries
 * @param[in] csr compressed sparse row of the local partition of the graph
 * @param[out] scc_id map vertex to strongly connected component id
 * @param[in] memory temp memory of:
 *   - 3 * page_ceil(local_n * sizeof(vertex_t))                 // index, low, st
 *   - page_ceil(local_n * sizeof(vertex_t) + sizeof(index_t)) // dfs stack
 *   - page_ceil(ceil(local_n/64) * sizeof(u64))                 // on_stack bitset
 * @return number of strongly connected components
 */
template<WorldPartConcept Part, typename Callback, typename Filter = decltype(no_filter)>
void
tarjan(Part const& part, index_t const* head, vertex_t const* csr, Callback callback, Filter filter = no_filter, void* memory = nullptr) noexcept
{
  KASPAN_STATISTIC_SCOPE("tarjan");
  constexpr auto index_undecided = scc_id_undecided;
  struct Frame
  {
    vertex_t local_u;
    index_t  it;
  };

  auto const local_n = part.local_n();

  Buffer buffer;
  if (memory == nullptr) {
    buffer = make_buffer<vertex_t, vertex_t, vertex_t, Frame>(
      local_n, local_n, local_n, local_n);
    memory = buffer.data();
  }

  vertex_t index_count = 0;

  auto index    = borrow_array_filled<vertex_t>(&memory, index_undecided, local_n);
  auto low      = borrow_array_clean<vertex_t>(&memory, local_n);
  auto on_stack = make_bits_clean(local_n);
  auto st       = make_stack<vertex_t>(local_n);
  auto dfs      = make_stack<Frame>(local_n);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  for (vertex_t local_root = 0; local_root < local_n; ++local_root) {
    if (not filter(local_root))
      continue;

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

        if (part.has_local(v)) { // ignore non local edges
          auto const local_v = part.to_local(v);
          if (not filter(local_v))
            continue;

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
        auto const end = st.size();

        while (true) {
          auto const local_v = st.back();
          on_stack.unset(local_v);
          st.pop();
          if (local_v == local_u)
            break;
        }

        auto const begin = st.size();
        callback(st.data() + begin, st.data() + end);
      }

      dfs.pop();
      if (not dfs.empty()) {
        auto const local_p = dfs.back().local_u;
        low[local_p]       = std::min(low[local_p], low[local_u]);
      }
    }
  }
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
template<typename Callback, typename Filter = decltype(no_filter)>
void
tarjan(vertex_t const n, index_t const* head, vertex_t const* csr, Callback callback, Filter filter = no_filter, void* memory = nullptr) noexcept
{
  tarjan(SingleWorldPart{ n }, head, csr, callback, filter, memory);
}
