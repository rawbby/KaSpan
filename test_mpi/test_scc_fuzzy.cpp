#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/scc/async/scc.hpp>
#include <kaspan/scc/scc.hpp>
#include <kaspan/test/fuzzy.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <briefkasten/noop_indirection.hpp>

#include <cmath>

using namespace kaspan;

int
main(
  int    argc,
  char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  for (int run = 1; run <= 40; ++run) {
    for (vertex_t n : { 1, 6, 8, 16, 33, 64 }) {

      DEBUG_ASSERT_EQ(n, mpi_basic::allreduce_single(n, mpi_basic::min));
      DEBUG_ASSERT_EQ(n, mpi_basic::allreduce_single(n, mpi_basic::max));

      auto const seed           = mpi_basic::allreduce_max_time();
      auto const part           = balanced_slice_part{ n };
      auto const [scc_id_, bgp] = fuzzy_local_scc_id_and_graph(seed, part);

      // Test sync version
      {
        auto const local_n = part.local_n();
        auto       scc_id  = make_array<vertex_t>(local_n);
        scc(bgp.view(), scc_id.data());
        test_validate_scc_id(bgp.fw_view(), scc_id_.data(), scc_id.data());
      }

      // Test async version with NoopIndirectionScheme
      {
        auto const local_n = part.local_n();
        auto       scc_id  = make_array<vertex_t>(local_n);
        async::scc<briefkasten::NoopIndirectionScheme>(bgp.view(), scc_id.data());
        test_validate_scc_id(bgp.fw_view(), scc_id_.data(), scc_id.data());
      }
    }
  }
}
