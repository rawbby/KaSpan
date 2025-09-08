#include <ispan/scc_detection.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Util.hpp>

int
main(int argc, char** argv)
{
  constexpr int npc      = 3;
  constexpr int npv[npc] = { 1, 2, 4 };
  mpi_sub_process(argc, argv, npc, npv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  int world_rank = 0;
  int world_size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  graph            g;
  constexpr size_t n = 7;
  constexpr size_t m = 11;
  g.vert_count       = n;
  g.edge_count       = m;

  auto fw_beg_pos = page_aligned_alloc((n + 1) * sizeof(index_t));
  ASSERT(fw_beg_pos.has_value());
  g.fw_beg = static_cast<index_t*>(fw_beg_pos.value());

  auto bw_beg_pos = page_aligned_alloc((n + 1) * sizeof(index_t));
  ASSERT(bw_beg_pos.has_value());
  g.bw_beg = static_cast<index_t*>(bw_beg_pos.value());

  auto fw_csr = page_aligned_alloc(m * sizeof(vertex_t));
  ASSERT(fw_csr.has_value());
  g.fw_csr = static_cast<vertex_t*>(fw_csr.value());

  auto bw_csr = page_aligned_alloc(m * sizeof(vertex_t));
  ASSERT(bw_csr.has_value());
  g.bw_csr = static_cast<vertex_t*>(bw_csr.value());

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

  g.fw_beg[0] = 0;
  g.fw_beg[1] = 1;
  g.fw_beg[2] = 4;
  g.fw_beg[3] = 5;
  g.fw_beg[4] = 7;
  g.fw_beg[5] = 9;
  g.fw_beg[6] = 10;
  g.fw_beg[7] = 11;

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

  g.bw_beg[0] = 0;
  g.bw_beg[1] = 2;
  g.bw_beg[2] = 3;
  g.bw_beg[3] = 6;
  g.bw_beg[4] = 8;
  g.bw_beg[5] = 9;
  g.bw_beg[6] = 10;
  g.bw_beg[7] = 11;

  constexpr int alpha = 1;

  std::vector<index_t> scc_id;
  scc_detection(&g, alpha, world_rank, world_size, &scc_id);

  if (world_rank == 0) {
    std::cout << "scc_id_orig:  0 1 0 1 1 5 5" << std::endl;
    std::cout << "scc_id_ispan:";
    for (size_t i = 0; i < n; ++i)
      std::cout << ' ' << scc_id[i];
    std::cout << std::endl;
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
