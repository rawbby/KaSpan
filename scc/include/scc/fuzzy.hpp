#pragma once

#include <memory/stack_accessor.hpp>

#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <scc/partion_graph.hpp>
#include <memory/buffer.hpp>

#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <utility>
#include <vector>

struct LocalSccGraph
{
  Buffer buffer;
  Graph  graph;
  vertex_t*  scc_id;
};

/// memory = 4 * page_ceil(n * sizeof(vertex_t))
inline auto
fuzzy_global_scc_id_and_graph(u64 seed, u64 n, void*  memory, double degree = -1.0) -> Result<LocalSccGraph>
{
  if (degree < 0.0)
    degree = std::log(std::max(0.0, std::log(n)));

  auto rng       = std::mt19937{ seed };
  auto start_new = std::bernoulli_distribution{ 0.25 };

  auto scc_id    = ::borrow<vertex_t>(memory, n);
  auto fw_degree = ::borrow_clean<vertex_t>(memory, n);
  auto bw_degree = ::borrow_clean<vertex_t>(memory, n);

  auto comps = std::map<u64, std::vector<u64>>{};

  {
    auto reps = StackAccessor<vertex_t>::borrow(memory, n);
    for (u64 v = 0; v < n; ++v) {
      if (v == 0 or start_new(rng)) {

        reps.push(v);
        comps[v].push_back(v);

        scc_id[v] = v;

      } else {

        auto       pick = std::uniform_int_distribution<size_t>{ 0, reps.size() - 1 };
        auto const p    = pick(rng);

        auto const root = reps[p];

        scc_id[v] = root;
        comps[root].push_back(v);
      }
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

  LocalSccGraph result;
  RESULT_TRY(result.buffer, Buffer::create(2 * page_ceil((n + 1) * sizeof(index_t)) + 2 * page_ceil(m * sizeof(vertex_t)) + page_ceil(n * sizeof(vertex_t))));
  auto access = result.buffer.data();

  result.scc_id = ::borrow<vertex_t>(access, n);
  std::memcpy(result.scc_id, scc_id, n * sizeof(vertex_t));

  result.graph.n       = n;
  result.graph.m       = m;
  result.graph.fw_head = ::borrow<index_t>(memory, n + 1);
  result.graph.bw_head = ::borrow<index_t>(memory, n + 1);
  result.graph.fw_csr  = ::borrow<vertex_t>(memory, m);
  result.graph.bw_csr  = ::borrow<vertex_t>(memory, m);

  {
    u64 pos                 = 0;
    result.graph.fw_head[0] = pos;
    for (u64 i = 0; i < n; ++i) {
      pos += fw_degree[i];
      result.graph.fw_head[i + 1] = pos;
    }
    pos                     = 0;
    result.graph.bw_head[0] = pos;
    for (u64 i = 0; i < n; ++i) {
      pos += bw_degree[i];
      result.graph.bw_head[i + 1] = pos;
    }
    std::vector<u64> fw_pos(n), bw_pos(n);
    for (u64 i = 0; i < n; ++i) {
      fw_pos[i] = result.graph.fw_head[i];
      bw_pos[i] = result.graph.bw_head[i];
    }
    for (auto const& [u, v] : edges) {
      result.graph.fw_csr[fw_pos[u]++] = v;
      result.graph.bw_csr[bw_pos[v]++] = u;
    }
  }

  return result;
}

/// memory = 4 * page_ceil(n * sizeof(vertex_t))
template<WorldPartConcept Part>
auto
fuzzy_local_scc_id_and_graph(u64 seed, Part const& part, void*  memory, double degree = -1.0) -> Result<LocalSccGraphPart<Part>>
{
  RESULT_TRY(auto fuzzy_scc_graph, fuzzy_global_scc_id_and_graph(seed, part.n, memory, degree));
  return partition(fuzzy_scc_graph.graph, fuzzy_scc_graph.scc_id, part);
}
