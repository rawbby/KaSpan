#pragma once

#include "kaspan/graph/bidi_graph.hpp"

#include <kaspan/graph/graph.hpp>
#include <kaspan/memory/accessor/stack.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/partion_graph.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <algorithm>
#include <map>
#include <random>
#include <ranges>
#include <set>
#include <utility>
#include <vector>

namespace kaspan {

inline auto
fuzzy_global_scc_id_and_graph(
  u64      seed,
  vertex_t n,
  double   d = -1.0)
{
  DEBUG_ASSERT_GE(n, 0);
  DEBUG_ASSERT(d == -1.0 or d >= 0.0);

  auto rng = std::mt19937{ seed };

  // normalize degree
  d = [&rng, n, d] {
    // corner cases
    if (n < 2) return -1.0;

    auto const min_d = std::log(std::max(1.0, std::log(static_cast<double>(n))));
    auto const max_d = static_cast<double>(n - 1) / 2.0; // this is not the upper bound, but a reasonable limit performance wise

    // random request
    if (d == -1.0) return std::uniform_real_distribution{ min_d, max_d }(rng);

    // logical clamping
    if (d < min_d) return min_d;
    if (d > max_d) return max_d;
    return d;
  }();

  auto scc_id    = make_array_clean<vertex_t>(n);
  auto fw_degree = make_array_clean<vertex_t>(n);
  auto bw_degree = make_array_clean<vertex_t>(n);
  auto start_new = std::bernoulli_distribution{ 0.25 };

  auto comps = std::map<vertex_t, std::vector<vertex_t>>{};
  auto reps  = make_stack<vertex_t>(n);
  for (vertex_t v = 0; v < n; ++v) {
    if (v == 0 or start_new(rng)) {
      reps.push(v);
      comps[v].push_back(v);
      scc_id[v] = v;
    } else {
      auto       pick = std::uniform_int_distribution<size_t>(0, reps.size() - 1);
      auto const root = reps.data()[pick(rng)];
      scc_id[v]       = root;
      comps[root].push_back(v);
    }
  }

  auto edges        = std::set<edge_t>{};
  auto try_add_edge = [&](auto u, auto v) {
    if (u != v and edges.emplace(u, v).second) {
      ++fw_degree[u];
      ++bw_degree[v];
    }
  };

  // create random cyclic path in
  // each non singular component
  for (auto& comp : comps | std::views::values) {
    if (comp.size() > 1) {
      std::ranges::shuffle(comp, rng);
      try_add_edge(comp.front(), comp.back());
      for (size_t i = 1; i < comp.size(); ++i) {
        try_add_edge(comp[i], comp[i - 1]);
      }
    }
  }

  auto avg_degree = [&] {
    if (n == 0) return 0.0;
    return static_cast<double>(edges.size()) / n;
  };

  std::vector<vertex_t> prior;
  prior.reserve(comps.size());
  while (avg_degree() < d) {
    prior.clear();

    for (auto& [id, comp_u] : comps) {
      prior.emplace_back(id);

      auto pick_u      = std::uniform_int_distribution<size_t>{ 0, comp_u.size() - 1 };
      auto pick_comp_v = std::uniform_int_distribution<size_t>{ 0, prior.size() - 1 };

      for (size_t i = 0; i < comp_u.size(); ++i) {
        if (avg_degree() >= d) {
          break;
        }

        auto const& comp_v = comps[prior[pick_comp_v(rng)]];
        auto        pick_v = std::uniform_int_distribution<size_t>{ 0, comp_v.size() - 1 };

        auto const u = comp_u[pick_u(rng)];
        auto const v = comp_v[pick_v(rng)];

        try_add_edge(u, v);
      }
    }
  }

  auto bg = bidi_graph(n, integral_cast<index_t>(edges.size()));

  if (n > 0) {
    index_t offset = 0;
    bg.fw.head[0]  = offset;
    for (vertex_t i = 0; i < n; ++i) {
      offset += fw_degree[i];
      bg.fw.head[i + 1] = offset;
    }
    std::vector<index_t> fw_pos(n);
    for (vertex_t i = 0; i < n; ++i) {
      fw_pos[i] = bg.fw.head[i];
    }
    for (auto const& [u, v] : edges) {
      bg.fw.csr[fw_pos[u]++] = v;
    }
  }

  if (n > 0) {
    index_t offset = 0;
    bg.bw.head[0]  = offset;
    for (vertex_t i = 0; i < n; ++i) {
      offset += bw_degree[i];
      bg.bw.head[i + 1] = offset;
    }

    std::vector<index_t> bw_pos(n);
    for (vertex_t i = 0; i < n; ++i) {
      bw_pos[i] = bg.bw.head[i];
    }
    for (auto const& [u, v] : edges) {
      bg.bw.csr[bw_pos[v]++] = u;
    }
  }

  bg.debug_validate();
  return PACK(scc_id, bg);
}

template<world_part_concept part_t>
auto
fuzzy_local_scc_id_and_graph(
  u64           seed,
  part_t const& part,
  double        degree = -1.0)
{
  auto const local_n = part.local_n();

  auto [scc_id, bg] = fuzzy_global_scc_id_and_graph(seed, part.n, degree);

  auto scc_id_part = make_array<vertex_t>(local_n);
  auto bgp         = partition(bg.view(), part);

  bgp.each_k([&](auto k) {
    scc_id_part[k] = scc_id[part.to_global(k)];
  });

  return PACK(scc_id_part, bgp);
}

} // namespace kaspan
