#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <scc/allgather_graph.hpp>
#include <scc/fuzzy.hpp>
#include <scc/partion_graph.hpp>
#include <util/scope_guard.hpp>

void
check_gp(auto const& gp)
{
  auto const& p = gp.part;

  auto const n = p.n;
  auto const m = gp.m;

  auto const local_n    = p.local_n();
  auto const local_fw_m = gp.local_fw_m;
  auto const local_bw_m = gp.local_bw_m;

  ASSERT_GE(n, local_n);
  ASSERT_GE(local_n, 0);

  ASSERT_GE(m, 0);
  ASSERT_GE(local_fw_m, 0);
  ASSERT_GE(local_bw_m, 0);

  if (local_n > 0) {
    ASSERT_EQ(gp.fw_head[0], 0);
    ASSERT_EQ(gp.bw_head[0], 0);
    ASSERT_EQ(gp.fw_head[local_n], local_fw_m);
    ASSERT_EQ(gp.bw_head[local_n], local_bw_m);
  }
  for (vertex_t k = 0; k < local_n; ++k) {
    ASSERT_GE(gp.fw_head[k + 1], gp.fw_head[k], "k=%d", k);
    ASSERT_GE(gp.bw_head[k + 1], gp.bw_head[k], "k=%d", k);
  }
  for (index_t it = 0; it < local_fw_m; ++it) {
    ASSERT_GE(gp.fw_csr[it], 0, "it=%d", it);
    ASSERT_LT(gp.fw_csr[it], n, "it=%d n=%d", it, n);
  }
  for (index_t it = 0; it < local_bw_m; ++it) {
    ASSERT_GE(gp.bw_csr[it], 0, "it=%d", it);
    ASSERT_LT(gp.bw_csr[it], n, "it=%d n=%d", it, n);
  }
}

void
check_g(auto const& g)
{
  auto const n = g.n;
  auto const m = g.m;

  ASSERT_GE(n, 0);
  ASSERT_GE(m, 0);

  if (n > 0) {
    ASSERT_EQ(g.fw_head[0], 0);
    ASSERT_EQ(g.bw_head[0], 0);
    ASSERT_EQ(g.fw_head[n], m);
    ASSERT_EQ(g.bw_head[n], m);
  }
  for (vertex_t u = 0; u < n; ++u) {
    ASSERT_GE(g.fw_head[u + 1], g.fw_head[u], "k=%d", u);
    ASSERT_GE(g.bw_head[u + 1], g.bw_head[u], "k=%d", u);
  }
  for (index_t it = 0; it < m; ++it) {
    ASSERT_GE(g.fw_csr[it], 0, "it=%d", it);
    ASSERT_LT(g.fw_csr[it], n, "it=%d n=%d", it, n);
  }
  for (index_t it = 0; it < m; ++it) {
    ASSERT_GE(g.bw_csr[it], 0, "it=%d", it);
    ASSERT_LT(g.bw_csr[it], n, "it=%d n=%d", it, n);
  }
}

void
check_p(auto const& p)
{
  ASSERT_GE(p.local_n(), 0);
  ASSERT_GE(p.n, p.local_n());

  ASSERT_GE(p.begin, 0);
  ASSERT_GE(p.end, p.begin);
  ASSERT_GE(p.n, p.end);

  ASSERT_GE(p.world_rank, 0);
  ASSERT_GT(p.world_size, p.world_rank);
}

void
check_p_p(auto const& p, auto const& p_)
{
  check_p(p);
  ASSERT_EQ(p.n, p_.n);
  ASSERT_EQ(p.local_n(), p_.local_n());
  ASSERT_EQ(p.begin, p_.begin);
  ASSERT_EQ(p.end, p_.end);
  ASSERT_EQ(p.world_rank, p_.world_rank);
  ASSERT_EQ(p.world_size, p_.world_size);
  for (vertex_t k = 0; k < p.local_n(); ++k) {
    ASSERT_EQ(p.to_global(k), p.to_global(k), "k=%d", k);
  }
}

void
check_gp_gp(auto const& gp, auto const& gp_)
{
  auto const& p = gp.part;
  check_p_p(p, gp_.part);

  auto const local_n = p.local_n();

  ASSERT_EQ(gp.m, gp_.m);
  ASSERT_EQ(gp.local_fw_m, gp_.local_fw_m);
  ASSERT_EQ(gp.local_bw_m, gp_.local_bw_m);

  for (vertex_t k = 0; k <= local_n; ++k) {
    ASSERT_EQ(gp.fw_head[k], gp_.fw_head[k], "k=%d", k);
    ASSERT_EQ(gp.bw_head[k], gp_.bw_head[k], "k=%d", k);
  }
  for (index_t it = 0; it < gp.local_fw_m; ++it) {
    ASSERT_EQ(gp.fw_csr[it], gp_.fw_csr[it], "it=%d", it);
  }
  for (index_t it = 0; it < gp.local_bw_m; ++it) {
    ASSERT_EQ(gp.bw_csr[it], gp_.bw_csr[it], "it=%d", it);
  }
}

void
check_g_gp(auto const& g, auto const& gp)
{
  auto const& p       = gp.part;
  auto const  local_n = p.local_n();

  ASSERT_GE(g.n, local_n);
  ASSERT_EQ(g.m, gp.m);
  ASSERT_GE(g.m, gp.local_fw_m);
  ASSERT_GE(g.m, gp.local_bw_m);

  index_t gp_local_fw_m = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u = p.to_global(k);

    auto const g_beg = g.fw_head[u];
    auto const g_deg = g.fw_head[u + 1] - g_beg;

    auto const gp_beg = gp.fw_head[k];
    auto const gp_deg = gp.fw_head[k + 1] - gp_beg;

    ASSERT_EQ(g_deg, gp_deg);
    for (index_t i = 0; i < g_deg; ++i) {
      ASSERT_EQ(g.fw_csr[g_beg + i], gp.fw_csr[gp_beg + i], "i=%d", i);
    }

    gp_local_fw_m += gp_deg;
  }
  ASSERT_EQ(gp_local_fw_m, gp.local_fw_m);

  index_t gp_local_bw_m = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u = p.to_global(k);

    auto const g_beg = g.bw_head[u];
    auto const g_deg = g.bw_head[u + 1] - g_beg;

    auto const gp_beg = gp.bw_head[k];
    auto const gp_deg = gp.bw_head[k + 1] - gp_beg;

    ASSERT_EQ(g_deg, gp_deg);
    for (index_t i = 0; i < g_deg; ++i) {
      ASSERT_EQ(g.bw_csr[g_beg + i], gp.bw_csr[gp_beg + i], "i=%d", i);
    }

    gp_local_bw_m += gp_deg;
  }
  ASSERT_EQ(gp_local_bw_m, gp.local_bw_m);
}

void
check_g_g(auto const& g, auto const& g_)
{
  ASSERT_EQ(g.n, g_.n);
  ASSERT_EQ(g.m, g_.m);
  for (vertex_t u = 0; u <= g.n; ++u) {
    ASSERT_EQ(g.fw_head[u], g_.fw_head[u], "u=%d", u);
    ASSERT_EQ(g.bw_head[u], g_.bw_head[u], "u=%d", u);
  }
  for (index_t it = 0; it < g.m; ++it) {
    ASSERT_EQ(g.fw_csr[it], g_.fw_csr[it], "it=%d", it);
    ASSERT_EQ(g.bw_csr[it], g_.bw_csr[it], "it=%d", it);
  }
}

void
test_allgather_graph(auto const& g, auto const& p)
{
  // graph -> part -> graph -> part
  check_g(g);

  auto gp = partition(g.m, g.fw_head, g.fw_csr, g.bw_head, g.bw_csr, p);
  check_gp(gp);
  check_g_gp(g, gp);

  auto g_ = allgather_graph(p, gp.m, gp.local_fw_m, gp.fw_head, gp.fw_csr);
  check_g(g_);
  check_g_g(g, g_);
  check_g_gp(g_, gp);

  auto gp_ = partition(g_.m, g_.fw_head, g_.fw_csr, g_.bw_head, g_.bw_csr, p);
  check_gp(gp_);
  check_g_gp(g, gp_);
  check_g_gp(g_, gp_);
  check_gp_gp(gp, gp_);
}

int
main(int argc, char** argv)
{
  constexpr int npc = 1;
  constexpr int npv[1]{ 3 };
  mpi_sub_process(argc, argv, npc, npv);

  KASPAN_DEFAULT_INIT();

  constexpr auto n     = 1024;
  auto const     graph = fuzzy_global_scc_id_and_graph(1, n, 1);
  test_allgather_graph(graph, TrivialSlicePart{ n });
  test_allgather_graph(graph, BalancedSlicePart{ n });
}
