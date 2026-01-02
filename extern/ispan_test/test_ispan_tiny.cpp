#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <ispan/scc.hpp>
#include <scc/base.hpp>
#include <util/scope_guard.hpp>

#include <cstdio>
#include <iostream>
#include <print>

int
main(int argc, char** argv)
{
  constexpr int npc      = 3;
  constexpr int npv[npc] = { 1, 2, 4 };
  mpi_sub_process(argc, argv, npc, npv);

  KASPAN_DEFAULT_INIT();

  Graph            g;
  constexpr size_t n = 7;
  constexpr size_t m = 11;
  g.n                = n;
  g.m                = m;

  auto const buffer = make_graph_buffer(n, m);
  auto*      memory = buffer.data();

  g.fw_head = ::borrow_array<index_t>(&memory, n + 1);
  g.bw_head = ::borrow_array<index_t>(&memory, n + 1);
  g.fw_csr  = ::borrow_array<vertex_t>(&memory, m);
  g.bw_csr  = ::borrow_array<vertex_t>(&memory, m);

  g.fw_csr[0]  = 2; // 0 2
  g.fw_csr[1]  = 0; // 1 0
  g.fw_csr[2]  = 2; // 1 2
  g.fw_csr[3]  = 3; // 1 3
  g.fw_csr[4]  = 0; // 2 0
  g.fw_csr[5]  = 2; // 3 2
  g.fw_csr[6]  = 4; // 3 4
  g.fw_csr[7]  = 1; // 4 1
  g.fw_csr[8]  = 3; // 4 3
  g.fw_csr[9]  = 6; // 5 6
  g.fw_csr[10] = 5; // 6 5

  g.fw_head[0] = 0;
  g.fw_head[1] = 1;
  g.fw_head[2] = 4;
  g.fw_head[3] = 5;
  g.fw_head[4] = 7;
  g.fw_head[5] = 9;
  g.fw_head[6] = 10;
  g.fw_head[7] = 11;

  g.bw_csr[0]  = 1; // 0 1
  g.bw_csr[1]  = 2; // 0 2
  g.bw_csr[2]  = 4; // 1 4
  g.bw_csr[3]  = 0; // 2 0
  g.bw_csr[4]  = 1; // 2 1
  g.bw_csr[5]  = 3; // 2 3
  g.bw_csr[6]  = 1; // 3 1
  g.bw_csr[7]  = 4; // 3 4
  g.bw_csr[8]  = 3; // 4 3
  g.bw_csr[9]  = 6; // 5 6
  g.bw_csr[10] = 5; // 6 5

  g.bw_head[0] = 0;
  g.bw_head[1] = 2;
  g.bw_head[2] = 3;
  g.bw_head[3] = 6;
  g.bw_head[4] = 8;
  g.bw_head[5] = 9;
  g.bw_head[6] = 10;
  g.bw_head[7] = 11;

  constexpr int alpha = 1;

  std::vector<index_t> scc_id;
  scc(g.n, g.m, g.fw_head, g.fw_csr, g.bw_head, g.bw_csr, alpha, &scc_id);

  if (mpi_basic::world_rank == 0) {
    std::println("scc_id_orig:  0 1 0 1 1 5 5");
    std::print("scc_id_ispan:");
    for (size_t i = 0; i < n; ++i) std::print(" {}", scc_id[i]);
    std::println();
  }

  // SCC's:
  // - [0,2]
  // - [1,3,4]
  // - [5,6]
  ASSERT_EQ(scc_id[0], 0);
  ASSERT_EQ(scc_id[1], 1);
  ASSERT_EQ(scc_id[2], 0);
  ASSERT_EQ(scc_id[3], 1);
  ASSERT_EQ(scc_id[4], 1);
  ASSERT_EQ(scc_id[5], 5);
  ASSERT_EQ(scc_id[6], 5);
}
