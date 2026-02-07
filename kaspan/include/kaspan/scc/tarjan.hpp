#pragma once

#include <kaspan/memory/bits.hpp>
#include <kaspan/memory/stack.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/graph/single_part.hpp>

namespace kaspan {

constexpr auto
no_filter(
  vertex_t /* k */) -> bool
{
  return true;
}

template<part_view_c Part,
         typename callback_t,
         typename filter_t = decltype(no_filter)>
void
tarjan(
  graph_part_view<Part> gp,
  callback_t            callback,
  filter_t              filter        = no_filter,
  vertex_t              decided_count = 0) noexcept
{
  KASPAN_STATISTIC_SCOPE("tarjan");
  constexpr auto index_undecided = scc_id_undecided;
  struct frame
  {
    vertex_t local_u;
    index_t  it;
  };

  auto const local_n         = gp.part.local_n();
  auto const undecided_count = local_n - decided_count;

  vertex_t index_count = 0;

  auto index    = make_array_filled<vertex_t>(index_undecided, local_n);
  auto low      = make_array_clean<vertex_t>(local_n);
  auto on_stack = make_bits_clean(local_n);
  auto st       = make_stack<vertex_t>(undecided_count);
  auto dfs      = make_stack<frame>(undecided_count);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  for (vertex_t local_root = 0; local_root < local_n; ++local_root) {
    if (!filter(local_root)) {
      continue;
    }

    if (index[local_root] != index_undecided) {
      continue;
    }

    index[local_root] = low[local_root] = index_count++;
    st.push(local_root);
    on_stack.set(local_root);
    dfs.push({ local_root, gp.head[local_root] });

    while (!dfs.empty()) {

      auto [local_u, it] = dfs.back();
      if (it < gp.head[local_u + 1]) {
        auto const v = gp.csr[it];
        dfs.back().it += 1; // Update stack frame iterator

        if (gp.part.has_local(v)) { // ignore non local edges
          auto const local_v = gp.part.to_local(v);
          if (!filter(local_v)) {
            continue;
          }

          auto const v_index = index[local_v];

          if (v_index == index_undecided) {
            index[local_v] = low[local_v] = index_count++;
            st.push(local_v);
            on_stack.set(local_v);
            dfs.push({ local_v, gp.head[local_v] });
          }

          else if (on_stack.get(local_v) && v_index < low[local_u]) {
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
          if (local_v == local_u) {
            break;
          }
        }

        auto const begin = st.size();

        // notice: this is kind of a dirty trick, but it works
        // we use the poped part of the stack to call the callback
        KASPAN_MEMCHECK_MAKE_MEM_DEFINED(st.data() + begin, (end - begin) * sizeof(vertex_t));
        callback(st.data() + begin, st.data() + end);
        KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(st.data() + begin, (end - begin) * sizeof(vertex_t));
      }

      dfs.pop();
      if (!dfs.empty()) {
        auto const local_p = dfs.back().local_u;
        low[local_p]       = std::min(low[local_p], low[local_u]);
      }
    }
  }
}

template<typename Callback,
         typename Filter = decltype(no_filter)>
void
tarjan(
  graph_view g,
  Callback   callback,
  Filter     filter        = no_filter,
  vertex_t   decided_count = 0) noexcept
{
  auto const part = single_part{ g.n };
  auto const gv   = graph_part_view{ part.view(), g.m, g.head, g.csr };
  tarjan(gv, callback, filter, decided_count);
}

} // namespace kaspan
