#include <ispan/scc_detection.hpp>
#include <ispan/test/random_graph_from_scc.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Util.hpp>

#include <iostream>
#include <streambuf>

struct NullBuffer : std::streambuf
{
  int overflow(int c) override
  {
    return traits_type::not_eof(c);
  }
};

class CoutSilencer
{
  NullBuffer      nb_;
  std::streambuf* old_;

public:
  CoutSilencer()
    : old_(std::cout.rdbuf(&nb_))
  {
  }
  ~CoutSilencer()
  {
    std::cout.rdbuf(old_);
  }
};

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  int world_rank = 0;
  int world_size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  for (size_t n = 16; n < 48; n += 16) {

    auto const scc_id_orig = make_random_normalized_scc_id(n);
    auto const g           = make_random_graph_from_scc_ids(scc_id_orig);

    if (world_rank == 0 and n <= 128) {
      std::cout << "% g edgelist" << std::endl;

      for (size_t u = 0; u < g.vert_count; ++u) {
        for (size_t i = g.fw_beg_pos[u]; i < g.fw_beg_pos[u + 1]; ++i) {
          auto const v = g.fw_csr[i];
          std::cout << u << ' ' << v << std::endl;
        }
      }
    }

    constexpr int alpha        = 1;
    constexpr int beta         = 1;
    double        avg_time[15] = {};

    std::vector<index_t> scc_id_ispan;
    scc_id_ispan.resize(n);

    {
      CoutSilencer silence;
      scc_detection(&g, alpha, beta, avg_time, world_rank, world_size, 1, &scc_id_ispan);
    }

    if (world_rank == 0 and n <= 128) {
      std::cout << "scc_id_orig:    ";
      for (size_t i = 0; i < n; ++i)
        std::cout << ' ' << scc_id_orig[i];
      std::cout << std::endl;
      std::cout << "scc_id_ispan:   ";
      for (size_t i = 0; i < n; ++i)
        std::cout << ' ' << scc_id_ispan[i];
      std::cout << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    for (size_t i = 0; i < n; ++i)
      ASSERT_EQ(scc_id_orig[i], scc_id_ispan[i], "i = %lu", i);
  }
}
