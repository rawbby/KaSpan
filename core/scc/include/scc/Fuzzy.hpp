#pragma once

#include <graph/Graph.hpp>
#include <graph/GraphPart.hpp>
#include <graph/convert/Graph2GraphPart.hpp>

#include <buffer/Buffer.hpp>
#include <util/Util.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <random>
#include <set>
#include <utility>
#include <vector>

inline auto
fuzzy_global_scc_id_and_graph(u64 seed, u64 n, double degree = -1.0) -> Result<std::pair<U64Buffer, Graph>>
{
  if (degree < 0.0)
    degree = std::log(std::max(0.0, std::log(n)));

  auto rng       = std::mt19937{ seed };
  auto start_new = std::bernoulli_distribution{ 0.25 };

  RESULT_TRY(auto scc_id, U64Buffer::create(n));
  RESULT_TRY(auto fw_degree, U32Buffer::zeroes(n));
  RESULT_TRY(auto bw_degree, U32Buffer::zeroes(n));

  auto reps  = std::vector<u64>{};
  auto comps = std::map<u64, std::vector<u64>>{};
  auto edges = std::set<std::pair<u64, u64>>{};

  for (u64 v = 0; v < n; ++v) {
    if (v == 0 or start_new(rng)) {

      reps.push_back(v);
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

  auto g = Graph{};
  g.n    = n;
  g.m    = m;

  RESULT_TRY(g.fw_head, U64Buffer::create(n + 1));
  RESULT_TRY(g.bw_head, U64Buffer::create(n + 1));
  RESULT_TRY(g.fw_csr, U64Buffer::create(m));
  RESULT_TRY(g.bw_csr, U64Buffer::create(m));

  {
    u64 pos = 0;
    g.fw_head.set(0, pos);
    for (u64 i = 0; i < n; ++i) {
      pos += fw_degree[i];
      g.fw_head.set(i + 1, pos);
    }
    pos = 0;
    g.bw_head.set(0, pos);
    for (u64 i = 0; i < n; ++i) {
      pos += bw_degree[i];
      g.bw_head.set(i + 1, pos);
    }
    std::vector<u64> fw_pos(n), bw_pos(n);
    for (u64 i = 0; i < n; ++i) {
      fw_pos[i] = g.fw_head[i];
      bw_pos[i] = g.bw_head[i];
    }
    for (auto const& [u, v] : edges) {
      g.fw_csr.set(fw_pos[u]++, v);
      g.bw_csr.set(bw_pos[v]++, u);
    }
  }

  return std::pair{ std::move(scc_id), std::move(g) };
}

template<WorldPartConcept Part>
auto
fuzzy_local_scc_id_and_graph(u64 seed, Part part, double degree = -1.0) -> Result<std::pair<U64Buffer, GraphPart<Part>>>
{
  RESULT_TRY(auto global_scc_id_and_graph, fuzzy_global_scc_id_and_graph(seed, part.n, degree));
  auto [global_scc_id, global_graph] = std::move(global_scc_id_and_graph);

  RESULT_TRY(auto local_scc_id, U64Buffer::create(part.local_n()));
  for (size_t k = 0; k < part.local_n(); ++k)
    local_scc_id[k] = global_scc_id[part.to_global(k)];

  RESULT_TRY(auto local_graph, load_local(global_graph, std::move(part)));

  return std::pair{ std::move(local_scc_id), std::move(local_graph) };
}
