#include <kaspan/test/fuzzy.hpp>
#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/trivial_slice_part.hpp>
#include <kaspan/scc/allgather_graph.hpp>
#include <kaspan/scc/partion_graph.hpp>

using namespace kaspan;

template<part_view_concept Part>
void
check_p_p(
  Part p,
  Part p_)
{
  ASSERT_EQ(p.n(), p_.n());
  ASSERT_EQ(p.local_n(), p_.local_n());
  ASSERT_EQ(p.begin(), p_.begin());
  ASSERT_EQ(p.end(), p_.end());
  ASSERT_EQ(p.world_rank(), p_.world_rank());
  ASSERT_EQ(p.world_size(), p_.world_size());
  for (vertex_t k = 0; k < p.local_n(); ++k) {
    ASSERT_EQ(p.to_global(k), p.to_global(k), "k={}", k);
  }
}

template<part_view_concept Part>
void
check_gp_gp(
  bidi_graph_part_view<Part> bgpv,
  bidi_graph_part_view<Part> bgpv_)
{
  check_p_p(bgpv.part, bgpv_.part);
  ASSERT_EQ(bgpv.part.n(), bgpv_.part.n());
  ASSERT_EQ(bgpv.part.local_n(), bgpv_.part.local_n());
  ASSERT_EQ(bgpv.local_fw_m, bgpv_.local_fw_m);
  ASSERT_EQ(bgpv.local_bw_m, bgpv_.local_bw_m);
  ASSERT(
    std::ranges::equal(std::span{ bgpv.fw.head, integral_cast<size_t>(bgpv.part.local_n() + 1) }, std::span{ bgpv_.fw.head, integral_cast<size_t>(bgpv_.part.local_n() + 1) }));
  ASSERT(
    std::ranges::equal(std::span{ bgpv.bw.head, integral_cast<size_t>(bgpv.part.local_n() + 1) }, std::span{ bgpv_.bw.head, integral_cast<size_t>(bgpv_.part.local_n() + 1) }));
  ASSERT(std::ranges::equal(std::span{ bgpv.fw.csr, integral_cast<size_t>(bgpv.local_fw_m) }, std::span{ bgpv_.fw.csr, integral_cast<size_t>(bgpv_.local_fw_m) }));
  ASSERT(std::ranges::equal(std::span{ bgpv.bw.csr, integral_cast<size_t>(bgpv.local_bw_m) }, std::span{ bgpv_.bw.csr, integral_cast<size_t>(bgpv_.local_bw_m) }));
}

void
check_g_g(
  bidi_graph_view bgv,
  bidi_graph_view bgv_)
{
  ASSERT_EQ(bgv.n, bgv_.n);
  ASSERT_EQ(bgv.m, bgv_.m);
  ASSERT(std::ranges::equal(std::span{ bgv.fw.head, integral_cast<size_t>(bgv.n + 1) }, std::span{ bgv_.fw.head, integral_cast<size_t>(bgv_.n + 1) }));
  ASSERT(std::ranges::equal(std::span{ bgv.bw.head, integral_cast<size_t>(bgv.n + 1) }, std::span{ bgv_.bw.head, integral_cast<size_t>(bgv_.n + 1) }));
  ASSERT(std::ranges::equal(std::span{ bgv.fw.csr, integral_cast<size_t>(bgv.m) }, std::span{ bgv_.fw.csr, integral_cast<size_t>(bgv_.m) }));
  ASSERT(std::ranges::equal(std::span{ bgv.bw.csr, integral_cast<size_t>(bgv.m) }, std::span{ bgv_.bw.csr, integral_cast<size_t>(bgv_.m) }));
}

template<typename Part>
void
test_allgather_graph(
  bidi_graph_view bgv,
  Part const&     part)
{
  bgv.debug_validate();
  auto bgp = partition(bgv, part);
  bgp.debug_validate();

  auto bg_ = allgather_graph(bgv.m, bgp.fw_view());
  bg_.debug_validate();
  check_g_g(bgv, bg_.view());

  auto bgp_ = partition(bg_.view(), bgp.part);
  bgp_.debug_validate();
  check_gp_gp(bgp.view(), bgp_.view());
}

int
main(
  int    argc,
  char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  constexpr auto n        = 1024;
  auto const [scc_id, bg] = fuzzy_global_scc_id_and_graph(1, n, 1);

  test_allgather_graph(bg.view(), trivial_slice_part{ n });
  test_allgather_graph(bg.view(), balanced_slice_part{ n });
}
