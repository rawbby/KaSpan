#include "debug/assert_ge.hpp"
#include "mpi_basic/world.hpp"
#include "debug/assert_lt.hpp"
#include "debug/assert_eq.hpp"
#include "mpi_basic/allreduce_single.hpp"
#include "mpi_basic/allgather.hpp"
#include "scc/graph.hpp"
#include "memory/borrow.hpp"
#include "scc/base.hpp"
#include "scc/backward_complement.hpp"
#include "scc/part.hpp"
#include <debug/sub_process.hpp>
#include <scc/allgather_sub_graph.hpp>
#include <scc/partion_graph.hpp>

int
main(int argc, char** argv)
{
  constexpr int npc = 1;
  constexpr int npv[1]{ 3 };
  mpi_sub_process(argc, argv, npc, npv);

  KASPAN_DEFAULT_INIT();

  ASSERT_GE(mpi_basic::world_rank, 0);
  ASSERT_LT(mpi_basic::world_rank, mpi_basic::world_size);
  ASSERT_EQ(mpi_basic::world_size, 3);

  ASSERT_EQ(mpi_basic::allreduce_single(mpi_basic::world_rank, MPI_SUM), 0 + 1 + 2);

  LocalGraph g;
  g.n = 6;
  g.m = 10;

  g.buffer     = make_graph_buffer(g.n, g.m);
  auto* memory = g.buffer.data();
  g.fw_head    = borrow_array<index_t>(&memory, g.m);
  g.fw_csr     = borrow_array<vertex_t>(&memory, g.n + 1);
  g.bw_head    = borrow_array<index_t>(&memory, g.m);
  g.bw_csr     = borrow_array<vertex_t>(&memory, g.n + 1);

  g.fw_head[0]   = 0; //
  g.fw_head[1]   = 2; // *
  g.fw_head[2]   = 5; // *
  g.fw_head[3]   = 6; //
  g.fw_head[4]   = 6; // *
  g.fw_head[5]   = 9; //
  g.fw_head[g.n] = g.m;

  g.fw_csr[0] = 3; // 0 3
  g.fw_csr[1] = 4; // 0 4
  g.fw_csr[2] = 2; // 1 2 *
  g.fw_csr[3] = 4; // 1 4 *
  g.fw_csr[4] = 5; // 1 5
  g.fw_csr[5] = 4; // 2 4 *
  g.fw_csr[6] = 1; // 4 1 *
  g.fw_csr[7] = 3; // 4 3
  g.fw_csr[8] = 5; // 4 5
  g.fw_csr[9] = 0; // 5 0

  backward_complement_graph(g.n, g.fw_head, g.fw_csr, g.bw_head, g.bw_csr);

  ASSERT_EQ(g.bw_head[0], 0);
  ASSERT_EQ(g.bw_head[1], 1);
  ASSERT_EQ(g.bw_head[2], 2);
  ASSERT_EQ(g.bw_head[3], 3);
  ASSERT_EQ(g.bw_head[4], 5);
  ASSERT_EQ(g.bw_head[5], 8);
  ASSERT_EQ(g.bw_head[g.n], g.m);

  ASSERT_EQ(g.bw_csr[0], 5);
  ASSERT_EQ(g.bw_csr[1], 4);
  ASSERT_EQ(g.bw_csr[2], 1);
  ASSERT_EQ(g.bw_csr[3], 0);
  ASSERT_EQ(g.bw_csr[4], 4);
  ASSERT_EQ(g.bw_csr[5], 0);
  ASSERT_EQ(g.bw_csr[6], 1);
  ASSERT_EQ(g.bw_csr[7], 2);
  ASSERT_EQ(g.bw_csr[8], 1);
  ASSERT_EQ(g.bw_csr[9], 4);

  auto const gp = partition(g.m, g.fw_head, g.fw_csr, g.bw_head, g.bw_csr, BalancedSlicePart(g.n));

  ASSERT_EQ(gp.part.n, 6);
  ASSERT_EQ(gp.part.local_n(), 2);
  ASSERT_EQ(gp.m, 10);

  constexpr auto local_sub_n = 1; // one sub_n on every rank

  auto const sub_g = allgather_sub_graph(gp.part, local_sub_n, gp.fw_head, gp.fw_csr, gp.bw_head, gp.bw_csr, [&](vertex_t k) {
    auto const u = gp.part.to_global(k);
    return u == 1 or u == 2 or u == 4;
  });

  ASSERT_EQ(sub_g.n, 3);
  ASSERT_EQ(sub_g.m, 4);

  ASSERT_EQ(sub_g.ids_inverse[0], 1, "wrong inverse mapping");
  ASSERT_EQ(sub_g.ids_inverse[1], 2, "wrong inverse mapping");
  ASSERT_EQ(sub_g.ids_inverse[2], 4, "wrong inverse mapping");

  ASSERT_EQ(sub_g.fw_head[0], 0, "wrong head pointer");
  ASSERT_EQ(sub_g.fw_head[1], 2, "wrong head pointer");
  ASSERT_EQ(sub_g.fw_head[2], 3, "wrong head pointer");
  ASSERT_EQ(sub_g.fw_head[3], sub_g.m, "wrong head pointer");

  ASSERT_EQ(sub_g.fw_csr[0], 1, "wrong csr entry");
  ASSERT_EQ(sub_g.fw_csr[1], 2, "wrong csr entry");
  ASSERT_EQ(sub_g.fw_csr[2], 2, "wrong csr entry");
  ASSERT_EQ(sub_g.fw_csr[3], 0, "wrong csr entry");

  ASSERT_EQ(sub_g.bw_head[0], 0, "wrong head pointer");
  ASSERT_EQ(sub_g.bw_head[1], 1, "wrong head pointer");
  ASSERT_EQ(sub_g.bw_head[2], 2, "wrong head pointer");
  ASSERT_EQ(sub_g.bw_head[3], sub_g.m, "wrong head pointer");

  ASSERT_EQ(sub_g.bw_csr[0], 2, "wrong csr entry");
  ASSERT_EQ(sub_g.bw_csr[1], 0, "wrong csr entry");
  ASSERT_EQ(sub_g.bw_csr[2], 0, "wrong csr entry");
  ASSERT_EQ(sub_g.bw_csr[3], 1, "wrong csr entry");
}
