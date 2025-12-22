#pragma once

#include <memory/accessor/stack_accessor.hpp>

#include <memory/buffer.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <scc/partion_graph.hpp>

#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <utility>
#include <vector>

inline auto
fuzzy_global_scc_id_and_graph(u64 seed, u64 n, double degree = -1.0, void* temp_memory = nullptr)
{
  struct LocalSccGraph : LocalGraph
  {
    Buffer    scc_id_buffer;
    vertex_t* scc_id = nullptr;
  };

  LocalSccGraph result;
  result.scc_id_buffer = make_buffer<vertex_t>(n);
  result.scc_id        = static_cast<vertex_t*>(result.scc_id_buffer.data());

  Buffer temp_buffer;
  if (temp_memory == nullptr) {
    temp_buffer = make_buffer<vertex_t>(n, n, n);
    temp_memory = temp_buffer.data();
  }

  if (degree < 0.0)
    degree = std::log(std::max(0.0, std::log(n)));

  auto rng       = std::mt19937{ seed };
  auto start_new = std::bernoulli_distribution{ 0.25 };

  auto fw_degree = ::borrow_array_clean<vertex_t>(&temp_memory, n);
  auto bw_degree = ::borrow_array_clean<vertex_t>(&temp_memory, n);

  auto comps = std::map<u64, std::vector<u64>>{};
  auto reps  = StackAccessor<vertex_t>::borrow(&temp_memory, n);
  for (u64 v = 0; v < n; ++v) {
    if (v == 0 or start_new(rng)) {

      reps.push(v);
      comps[v].push_back(v);

      result.scc_id[v] = v;

    } else {

      auto       pick = std::uniform_int_distribution<size_t>{ 0, reps.size() - 1 };
      auto const p    = pick(rng);

      auto const root = reps[p];

      result.scc_id[v] = root;
      comps[root].push_back(v);
    }
  }

  auto edges        = std::set<std::pair<u64, u64>>{};
  auto try_add_edge = [&](u64 u, u64 v) {
    if (u != v and edges.emplace(u, v).second) {
      ++fw_degree[u];
      ++bw_degree[v];
    }
  };

  // create random cyclic path in
  // each non singular component
  for (auto& [_, comp] : comps) {
    if (comp.size() != 1) {
      std::ranges::shuffle(comp, rng);
      try_add_edge(comp.front(), comp.back());
      for (size_t i = 1; i < comp.size(); ++i)
        try_add_edge(comp[i], comp[i - 1]);
    }
  }

  auto avg_degree = [&] {
    auto const dn = static_cast<double>(n);
    auto const dm = static_cast<double>(edges.size());
    return dm / dn;
  };

  std::vector<u64> prior;
  prior.reserve(comps.size());
  while (avg_degree() > degree) {
    prior.clear();

    for (auto& [id, comp_u] : comps) {
      prior.emplace_back(id);

      auto pick_u      = std::uniform_int_distribution<size_t>{ 0, comp_u.size() - 1 };
      auto pick_comp_v = std::uniform_int_distribution<size_t>{ 0, prior.size() - 1 };

      for (size_t i = 0; i < comp_u.size(); ++i) {
        if (avg_degree() > degree)
          break;

        auto const& comp_v = comps[prior[pick_comp_v(rng)]];
        auto        pick_v = std::uniform_int_distribution<size_t>{ 0, comp_v.size() - 1 };

        auto const u = comp_u[pick_u(rng)];
        auto const v = comp_v[pick_v(rng)];

        try_add_edge(u, v);
      }
    }
  }

  auto const m = edges.size();

  result.buffer      = make_graph_buffer(n, m);
  auto* graph_memory = result.buffer.data();

  result.n       = n;
  result.m       = m;
  result.fw_head = borrow_array<index_t>(&graph_memory, n + 1);
  result.bw_head = borrow_array<index_t>(&graph_memory, n + 1);
  result.fw_csr  = borrow_array<vertex_t>(&graph_memory, m);
  result.bw_csr  = borrow_array<vertex_t>(&graph_memory, m);

  {
    u64 pos           = 0;
    result.fw_head[0] = pos;
    for (u64 i = 0; i < n; ++i) {
      pos += fw_degree[i];
      result.fw_head[i + 1] = pos;
    }
    pos               = 0;
    result.bw_head[0] = pos;
    for (u64 i = 0; i < n; ++i) {
      pos += bw_degree[i];
      result.bw_head[i + 1] = pos;
    }
    std::vector<u64> fw_pos(n), bw_pos(n);
    for (u64 i = 0; i < n; ++i) {
      fw_pos[i] = result.fw_head[i];
      bw_pos[i] = result.bw_head[i];
    }
    for (auto const& [u, v] : edges) {
      result.fw_csr[fw_pos[u]++] = v;
      result.bw_csr[bw_pos[v]++] = u;
    }
  }

  DEBUG_ASSERT_VALID_GRAPH(result.n, result.m, result.fw_head, result.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(result.n, result.m, result.bw_head, result.bw_csr);
  return result;
}

/// memory = 4 * page_ceil(n * sizeof(vertex_t))
template<WorldPartConcept Part>
auto
fuzzy_local_scc_id_and_graph(u64 seed, Part const& part, double degree = -1.0, void* memory = nullptr)
{
  struct LocalSccGraphPart : LocalGraphPart<Part>
  {
    Buffer    scc_id_part_buffer;
    vertex_t* scc_id_part = nullptr;
  };

  auto const local_n = part.local_n();

  auto g = fuzzy_global_scc_id_and_graph(seed, part.n, degree, memory);
  DEBUG_ASSERT_VALID_GRAPH(g.n, g.m, g.fw_head, g.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH(g.n, g.m, g.bw_head, g.bw_csr);

  LocalSccGraphPart gp;
  static_cast<LocalGraphPart<Part>&>(gp) = partition(g.m, g.fw_head, g.fw_csr, g.bw_head, g.bw_csr, part);
  gp.scc_id_part_buffer                  = make_buffer<vertex_t>(local_n);
  gp.scc_id_part                         = static_cast<vertex_t*>(gp.scc_id_part_buffer.data());
  for (vertex_t k = 0; k < local_n; ++k) {
    gp.scc_id_part[k] = g.scc_id[part.to_global(k)];
  }

  DEBUG_ASSERT_VALID_GRAPH_PART(gp.part, gp.fw_head, gp.fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(gp.part, gp.bw_head, gp.bw_csr);
  return gp;
}
