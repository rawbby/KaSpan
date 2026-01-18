#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/block_cyclic_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/cyclic_part.hpp>
#include <kaspan/graph/explicit_continuous_part.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/graph/trivial_slice_part.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>
#include <kaspan/scc/backward_complement.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/partion_graph.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <mpi.h>

using namespace kaspan;

int
main(
  int    argc,
  char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  ASSERT_GE(mpi_basic::world_rank, 0);
  ASSERT_LT(mpi_basic::world_rank, mpi_basic::world_size);

  if (mpi_basic::world_size == 3) {
    ASSERT_EQ(mpi_basic::allreduce_single(mpi_basic::world_rank, MPI_SUM), 0 + 1 + 2);

    auto bg_          = bidi_graph{ 6, 10 };
    bg_.fw.head[0]    = 0; //
    bg_.fw.head[1]    = 2; // *
    bg_.fw.head[2]    = 5; // *
    bg_.fw.head[3]    = 6; //
    bg_.fw.head[4]    = 6; // *
    bg_.fw.head[5]    = 9; //
    bg_.fw.head[bg_.n] = bg_.m;

    bg_.fw.csr[0] = 3; // 0 3
    bg_.fw.csr[1] = 4; // 0 4
    bg_.fw.csr[2] = 2; // 1 2 *
    bg_.fw.csr[3] = 4; // 1 4 *
    bg_.fw.csr[4] = 5; // 1 5
    bg_.fw.csr[5] = 4; // 2 4 *
    bg_.fw.csr[6] = 1; // 4 1 *
    bg_.fw.csr[7] = 3; // 4 3
    bg_.fw.csr[8] = 5; // 4 5
    bg_.fw.csr[9] = 0; // 5 0

    backward_complement_graph(bg_.view());

    ASSERT_EQ(bg_.bw.head[0], 0);
    ASSERT_EQ(bg_.bw.head[1], 1);
    ASSERT_EQ(bg_.bw.head[2], 2);
    ASSERT_EQ(bg_.bw.head[3], 3);
    ASSERT_EQ(bg_.bw.head[4], 5);
    ASSERT_EQ(bg_.bw.head[5], 8);
    ASSERT_EQ(bg_.bw.head[bg_.n], bg_.m);

    ASSERT_EQ(bg_.bw.csr[0], 5);
    ASSERT_EQ(bg_.bw.csr[1], 4);
    ASSERT_EQ(bg_.bw.csr[2], 1);
    ASSERT_EQ(bg_.bw.csr[3], 0);
    ASSERT_EQ(bg_.bw.csr[4], 4);
    ASSERT_EQ(bg_.bw.csr[5], 0);
    ASSERT_EQ(bg_.bw.csr[6], 1);
    ASSERT_EQ(bg_.bw.csr[7], 2);
    ASSERT_EQ(bg_.bw.csr[8], 1);
    ASSERT_EQ(bg_.bw.csr[9], 4);

    auto const bgp = partition(bg_.view(), balanced_slice_part{ bg_.n });

    ASSERT_EQ(bgp.part.local_n(), 2);
    ASSERT_EQ(bgp.part.n(), 6);

    constexpr auto local_sub_n = 1; // one sub_n on every rank

    auto const [super_ids, bg] = allgather_sub_graph(bgp.part, local_sub_n, bgp.fw.head, bgp.fw.csr, bgp.bw.head, bgp.bw.csr, [&](vertex_t k) {
      auto const u = bgp.part.to_global(k);
      return u == 1 or u == 2 or u == 4;
    });

    ASSERT_EQ(bg.n, 3);
    ASSERT_EQ(bg.m, 4);

    ASSERT_EQ(super_ids[0], 1, "wrong inverse mapping");
    ASSERT_EQ(super_ids[1], 2, "wrong inverse mapping");
    ASSERT_EQ(super_ids[2], 4, "wrong inverse mapping");

    ASSERT_EQ(bg.fw.head[0], 0, "wrong head pointer");
    ASSERT_EQ(bg.fw.head[1], 2, "wrong head pointer");
    ASSERT_EQ(bg.fw.head[2], 3, "wrong head pointer");
    ASSERT_EQ(bg.fw.head[3], bg.m, "wrong head pointer");

    ASSERT_EQ(bg.fw.csr[0], 1, "wrong csr entry");
    ASSERT_EQ(bg.fw.csr[1], 2, "wrong csr entry");
    ASSERT_EQ(bg.fw.csr[2], 2, "wrong csr entry");
    ASSERT_EQ(bg.fw.csr[3], 0, "wrong csr entry");

    ASSERT_EQ(bg.bw.head[0], 0, "wrong head pointer");
    ASSERT_EQ(bg.bw.head[1], 1, "wrong head pointer");
    ASSERT_EQ(bg.bw.head[2], 2, "wrong head pointer");
    ASSERT_EQ(bg.bw.head[3], bg.m, "wrong head pointer");

    ASSERT_EQ(bg.bw.csr[0], 2, "wrong csr entry");
    ASSERT_EQ(bg.bw.csr[1], 0, "wrong csr entry");
    ASSERT_EQ(bg.bw.csr[2], 0, "wrong csr entry");
    ASSERT_EQ(bg.bw.csr[3], 1, "wrong csr entry");
  }
}
