#pragma once

#include <ispan/graph.hpp>
#include <test/Assert.hpp>
#include <util/Util.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

inline std::vector<vertex_t>
make_random_normalized_scc_id(size_t n)
{
  std::vector<vertex_t> scc_id(n);
  if (n == 0)
    return scc_id;

  std::mt19937                rng(std::random_device{}());
  std::bernoulli_distribution start_new(0.25);

  std::vector<vertex_t> reps;
  reps.reserve(n);

  for (vertex_t v = 0; v < n; ++v) {
    if (v == 0 || start_new(rng)) {
      reps.push_back(v);
      scc_id[v] = v;
    } else {
      std::uniform_int_distribution<size_t> pick(0, reps.size() - 1);
      scc_id[v] = reps[pick(rng)];
    }
  }

  return scc_id;
}

inline graph
make_random_graph_from_scc_ids(std::vector<vertex_t> const& scc_id)
{
  graph      g{};
  auto const n = scc_id.size();
  g.vert_count = n;

  // Group vertices by SCC representative
  std::unordered_map<vertex_t, std::vector<vertex_t>> comps;
  comps.reserve(n);

  for (vertex_t v = 0; v < n; ++v)
    comps[scc_id[v]].push_back(v);

  std::vector<vertex_t> reps;
  reps.reserve(comps.size());

  for (auto [first, second] : comps)
    reps.push_back(first);
  std::ranges::sort(reps);

  // RNG and helpers
  std::mt19937                               rng(std::random_device{}());
  std::uniform_real_distribution             prob(0.0, 1.0);
  std::bernoulli_distribution                coin(0.5);
  std::unordered_set<uint64_t>               seen;
  std::vector<std::pair<vertex_t, vertex_t>> edges;
  edges.reserve(n * 3);

  auto add_edge = [&](vertex_t u, vertex_t v) {
    if (u != v) {
      auto const key = static_cast<uint64_t>(u) << 32 | static_cast<uint64_t>(v);
      if (seen.insert(key).second)
        edges.emplace_back(u, v);
    }
  };

  // Build intra-SCC edges: cycle + chords + random extras (avoid "just one cycle")
  for (auto r : reps) {
    auto& c = comps[r];
    if (c.size() > 1) {
      std::ranges::shuffle(c, rng);

      // basic cycle to ensure strong connectivity
      for (size_t i = 0; i < c.size(); ++i)
        add_edge(c[i], c[(i + 1) % c.size()]);

      // deterministic chords for small extra connectivity
      if (c.size() >= 3) {
        for (size_t i = 0; i < c.size(); ++i)
          add_edge(c[i], c[(i + 2) % c.size()]);
      }

      // randomized extra internal edges
      std::uniform_int_distribution<size_t> di(0, c.size() - 1);
      size_t                                extras = c.size();
      size_t                                trials = 0;
      while (extras && trials < c.size() * 6) {
        ++trials;
        vertex_t u = c[di(rng)], v = c[di(rng)];
        if (u != v && coin(rng)) {
          add_edge(u, v);
          --extras;
        }
      }
    }
  }

  // Build inter-SCC edges: chain across sorted representatives + extra forward edges (keeps condensation acyclic)
  for (size_t i = 0; i + 1 < reps.size(); ++i) {
    auto&                                 A = comps[reps[i]];
    auto&                                 B = comps[reps[i + 1]];
    std::uniform_int_distribution<size_t> da(0, A.size() - 1), db(0, B.size() - 1);
    add_edge(A[da(rng)], B[db(rng)]);
  }
  for (size_t i = 0; i < reps.size(); ++i) {
    for (size_t j = i + 1; j < reps.size(); ++j) {
      if (prob(rng) < 0.25) {
        auto &                                A = comps[reps[i]], &B = comps[reps[j]];
        std::uniform_int_distribution<size_t> da(0, A.size() - 1), db(0, B.size() - 1);
        add_edge(A[da(rng)], B[db(rng)]);
      }
    }
  }

  // Materialize CSR arrays
  g.edge_count    = edges.size();
  auto fw_beg_mem = page_aligned_alloc((n + 1) * sizeof(index_t));
  ASSERT(fw_beg_mem.has_value());
  g.fw_beg_pos    = static_cast<index_t*>(fw_beg_mem.value());
  auto bw_beg_mem = page_aligned_alloc((n + 1) * sizeof(index_t));
  ASSERT(bw_beg_mem.has_value());
  g.bw_beg_pos    = static_cast<index_t*>(bw_beg_mem.value());
  auto fw_csr_mem = page_aligned_alloc(g.edge_count * sizeof(vertex_t));
  ASSERT(fw_csr_mem.has_value());
  g.fw_csr        = static_cast<vertex_t*>(fw_csr_mem.value());
  auto bw_csr_mem = page_aligned_alloc(g.edge_count * sizeof(vertex_t));
  ASSERT(bw_csr_mem.has_value());
  g.bw_csr = static_cast<vertex_t*>(bw_csr_mem.value());

  std::vector<index_t> out_degree(n, 0), in_degree(n, 0);
  for (auto [first, second] : edges) {
    ++out_degree[first];
    ++in_degree[second];
  }

  g.fw_beg_pos[0] = 0;
  for (size_t i = 0; i < n; ++i)
    g.fw_beg_pos[i + 1] = g.fw_beg_pos[i] + out_degree[i];

  g.bw_beg_pos[0] = 0;
  for (size_t i = 0; i < n; ++i)
    g.bw_beg_pos[i + 1] = g.bw_beg_pos[i] + in_degree[i];

  std::vector<index_t> cur_fw(n), cur_bw(n);
  for (size_t i = 0; i < n; ++i) {
    cur_fw[i] = g.fw_beg_pos[i];
    cur_bw[i] = g.bw_beg_pos[i];
  }

  for (auto [first, second] : edges)
    g.fw_csr[cur_fw[first]++] = second;

  for (auto [first, second] : edges)
    g.bw_csr[cur_bw[second]++] = first;

  return g;
}
