#pragma once

#include <scc/Common.hpp>
#include <scc/PivotSelection.hpp>
#include <scc/SyncFwBw.hpp>
#include <util/Result.hpp>

#include <graph/AllGatherGraph.hpp>
#include <graph/GraphPart.hpp>

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/grid_indirection.hpp>

#include <algorithm>
#include <cstdio>

template<WorldPartConcept Part>
static void
local_trajan_dfs(
  size_t                 root,
  GraphPart<Part> const& graph,
  U64Buffer&             local_scc_id,
  std::vector<i32>&      t_discovered,
  std::vector<i32>&      t_parent,
  std::vector<u8>&       onstack,
  std::vector<size_t>&   stack,
  i32&                   t,
  u64&                   next_scc_id)
{

  t_discovered[root] = t_parent[root] = t++;

  stack.push_back(root);
  onstack[root] = 1;

  auto const beg = graph.fw_head.get(root);
  auto const end = graph.fw_head.get(root + 1);
  for (auto it = beg; it < end; ++it) {

    auto const u = graph.fw_csr.get(it);
    if (not graph.part.contains(u)) // skip edges leaving the part
      continue;

    auto const k = graph.part.rank(u);

    if (local_scc_id.get(k) != scc_id_undecided)
      continue; // already decided

    if (t_discovered[k] == -1) {
      local_trajan_dfs(k, graph, local_scc_id, t_discovered, t_parent, onstack, stack, t, next_scc_id);
      t_parent[root] = std::min(t_parent[root], t_parent[k]);
    } else if (onstack[k]) {
      t_parent[root] = std::min(t_parent[root], t_discovered[k]);
    }
  }

  if (t_parent[root] == t_discovered[root]) {
    while (true) {
      auto const w = stack.back();
      stack.pop_back();
      onstack[w] = 0;

      local_scc_id.set(w, next_scc_id);
      if (w == root)
        break;
    }
    ++next_scc_id;
  }
}

template<WorldPartConcept Part>
static void
local_trajan(
  GraphPart<Part> const& graph,
  U64Buffer&             local_scc_id,
  std::vector<i32>&      t_discovered,
  std::vector<i32>&      t_parent,
  std::vector<u8>&       onstack,
  std::vector<size_t>&   stack,
  i32&                   t,
  u64&                   next_scc_id)
{
  for (size_t root = 0; root < graph.part.size(); ++root) {
    if (local_scc_id.get(root) == scc_id_undecided && t_discovered[root] == -1) {
      local_trajan_dfs(root, graph, local_scc_id, t_discovered, t_parent, onstack, stack, t, next_scc_id);
    }
  }
}

template<WorldPartConcept Part>
VoidResult
local_scc_detection(GraphPart<Part> const& graph, U64Buffer& local_scc_id)
{
  auto const n = graph.part.size();
  if (n == 0)
    return {};

  // Tarjan buffers
  std::vector t_discovered(n, -1);
  std::vector t_parent(n, -1);
  u64         next_scc_id = 0;

  std::vector<u8>     onstack(n, 0);
  std::vector<size_t> stack;
  stack.reserve(n);

  i32 t = 0;
  local_trajan(graph, local_scc_id, t_discovered, t_parent, onstack, stack, t, next_scc_id);

  return VoidResult::success();
}
