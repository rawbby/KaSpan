#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/fuzzy.hpp>
#include <kaspan/scc/tarjan.hpp>

#include <algorithm>
#include <cmath>
#include <random>

using namespace kaspan;

void
verify(
  graph_view      g,
  vertex_t const* scc_id,
  double          d)
{
  g.debug_validate();
  if (g.n > 1 and d >= 0.0) {
    ASSERT_GE(static_cast<double>(g.m) / g.n, d, "fuzzy generator should generate at least an average degree of d");
  }
  tarjan(g, [&](auto const* beg, auto const* end) {
    auto id = *std::min_element(beg, end);
    for (auto u : std::span{ beg, end }) {
      ASSERT_EQ(scc_id[u], id, "invalid instance was generated");
    }
  });
}

int
main()
{
  constexpr auto clamp_degree = [](vertex_t n, double d) {
    if (n < 2 or d == -1) {
      return -1.0;
    }
    auto const min_d = std::log(std::max(1.0, std::log(static_cast<double>(n))));
    auto const max_d = static_cast<double>(n - 1) / 2.0;
    if (d < min_d) {
      return min_d;
    }
    if (d > max_d) {
      return max_d;
    }
    return d;
  };

  auto rng         = std::mt19937{ std::random_device{}() };
  auto n_dist      = std::uniform_int_distribution{ 50, 400 };
  auto degree_dist = std::uniform_real_distribution{ 0.0, 7.0 };

  for (vertex_t n = 1; n < 50; ++n) {
    constexpr auto ignore_degree = -1.0;
    auto const [scc_id, bg]      = fuzzy_global_scc_id_and_graph(rng(), n, ignore_degree);
    verify(bg.fw_view(), scc_id.data(), ignore_degree);
    verify(bg.bw_view(), scc_id.data(), ignore_degree);
  }

  for (vertex_t n = 1; n < 50; n += 5) {
    for (double d = 0.0; d < 3.25; d += 0.70) {
      auto const d_           = clamp_degree(n, d);
      auto const [scc_id, bg] = fuzzy_global_scc_id_and_graph(rng(), n, d_);
      verify(bg.fw_view(), scc_id.data(), d_);
      verify(bg.bw_view(), scc_id.data(), d_);
    }
  }

  for (int i = 1; i < 100; ++i) {
    auto const n            = n_dist(rng);
    auto const d            = clamp_degree(n, degree_dist(rng));
    auto const [scc_id, bg] = fuzzy_global_scc_id_and_graph(rng(), n, d);
    verify(bg.fw_view(), scc_id.data(), d);
    verify(bg.bw_view(), scc_id.data(), d);
  }
}
