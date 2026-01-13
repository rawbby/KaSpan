#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/assert_ge.hpp>
#include <kaspan/debug/assert_ne.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/fuzzy.hpp>
#include <kaspan/scc/graph.hpp>
#include <kaspan/scc/tarjan.hpp>

#include <algorithm>
#include <cmath>
#include <random>

using namespace kaspan;

void
verify_graph(vertex_t n, index_t m, index_t const* head, vertex_t const* csr, vertex_t const* scc_id, double d)
{
  DEBUG_ASSERT_VALID_GRAPH(n, head, csr);
  for (vertex_t u = 0; u < n; ++u) {
    for (auto v : csr_range(head, csr, u)) {
      ASSERT_NE(u, v, "fuzzy generator is not supposed to generate self loops");
    }
  }

  if (n > 0 and d >= 0.0) {
    ASSERT_GE(static_cast<double>(m) / n, d, "fuzzy generator should generate at least an average degree of d");
  }

  tarjan(n, head, csr, [&](auto const* beg, auto const* end) {
    vertex_t min_id = *std::min_element(beg, end);
    for (auto const* it = beg; it != end; ++it) {
      ASSERT_EQ(scc_id[*it], min_id, "invalid instance was generated");
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
    auto const g = fuzzy_global_scc_id_and_graph(rng(), n);
    verify_graph(g.n, g.m, g.fw_head, g.fw_csr, g.scc_id, clamp_degree(n, 0.0));
  }

  for (vertex_t n = 1; n < 50; n += 5) {
    for (double d = 0.0; d < 3.25; d += 0.70) {
      auto const g = fuzzy_global_scc_id_and_graph(rng(), n, clamp_degree(n, d));
      verify_graph(g.n, g.m, g.fw_head, g.fw_csr, g.scc_id, clamp_degree(n, d));
    }
  }

  for (int i = 1; i < 100; ++i) {
    auto const n = n_dist(rng);
    auto const d = clamp_degree(n, degree_dist(rng));
    auto const g = fuzzy_global_scc_id_and_graph(rng(), n, d);
    verify_graph(g.n, g.m, g.fw_head, g.fw_csr, g.scc_id, d);
  }
}
