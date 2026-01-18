#include <kaspan/graph/base.hpp>
#include <ispan/scc.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <cstdio>
#include <print>

using namespace kaspan;

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);

  KASPAN_DEFAULT_INIT();

  constexpr size_t n = 7;
  constexpr size_t m = 11;
  bidi_graph       g(n, m);

  g.fw.csr[0]  = 2; // 0 2
  g.fw.csr[1]  = 0; // 1 0
  g.fw.csr[2]  = 2; // 1 2
  g.fw.csr[3]  = 3; // 1 3
  g.fw.csr[4]  = 0; // 2 0
  g.fw.csr[5]  = 2; // 3 2
  g.fw.csr[6]  = 4; // 3 4
  g.fw.csr[7]  = 1; // 4 1
  g.fw.csr[8]  = 3; // 4 3
  g.fw.csr[9]  = 6; // 5 6
  g.fw.csr[10] = 5; // 6 5

  g.fw.head[0] = 0;
  g.fw.head[1] = 1;
  g.fw.head[2] = 4;
  g.fw.head[3] = 5;
  g.fw.head[4] = 7;
  g.fw.head[5] = 9;
  g.fw.head[6] = 10;
  g.fw.head[7] = 11;

  g.bw.csr[0]  = 1; // 0 1
  g.bw.csr[1]  = 2; // 0 2
  g.bw.csr[2]  = 4; // 1 4
  g.bw.csr[3]  = 0; // 2 0
  g.bw.csr[4]  = 1; // 2 1
  g.bw.csr[5]  = 3; // 2 3
  g.bw.csr[6]  = 1; // 3 1
  g.bw.csr[7]  = 4; // 3 4
  g.bw.csr[8]  = 3; // 4 3
  g.bw.csr[9]  = 6; // 5 6
  g.bw.csr[10] = 5; // 6 5

  g.bw.head[0] = 0;
  g.bw.head[1] = 2;
  g.bw.head[2] = 3;
  g.bw.head[3] = 6;
  g.bw.head[4] = 8;
  g.bw.head[5] = 9;
  g.bw.head[6] = 10;
  g.bw.head[7] = 11;

  constexpr int alpha = 1;

  std::vector<index_t> scc_id;
  scc(g.n, g.m, g.fw.head, g.fw.csr, g.bw.head, g.bw.csr, alpha, &scc_id);

  if (mpi_basic::world_rank == 0) {
    std::println("scc_id_orig:  0 1 0 1 1 5 5");
    std::print("scc_id_ispan:");
    for (size_t i = 0; i < n; ++i)
      std::print(" {}", scc_id[i]);
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
