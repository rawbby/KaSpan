#include <ispan/scc_detection.hpp>
#include <ispan/test/random_graph_from_scc.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Util.hpp>

#include <iomanip>
#include <iostream>
#include <streambuf>

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

  for (size_t n = 4; n < 1024; n += 8) {

    auto const seed = std::bit_cast<u64>(mpi_global_max_wtime());

    auto const scc_id_orig = make_random_normalized_scc_id(n, seed);
    auto const g           = make_random_graph_from_scc_ids(scc_id_orig, seed);

    constexpr int alpha = 0; // always top down

    std::vector<index_t> scc_id_ispan;
    scc_detection(&g, alpha, world_rank, world_size, &scc_id_ispan);

    std::stringstream ss;
    ss << "  index        :";
    for (size_t i = 0; i < n; ++i)
      ss << ' ' << std::right << std::setw(2) << i;
    ss << "\n  scc_id_orig  :";
    for (size_t i = 0; i < n; ++i)
      ss << ' ' << std::right << std::setw(2) << scc_id_orig[i];
    ss << "\n  scc_id_ispan :";
    for (size_t i = 0; i < n; ++i)
      ss << ' ' << std::right << std::setw(2) << scc_id_ispan[i];
    ss << "\n                ";
    for (size_t i = 0; i < n; ++i)
      ss << (scc_id_orig[i] == scc_id_ispan[i] ? "   " : " ^^");
    ss << std::endl;

    auto const status_str = ss.str();

    MPI_Barrier(MPI_COMM_WORLD);

    for (size_t i = 0; i < n; ++i)
      ASSERT_EQ(scc_id_orig[i], scc_id_ispan[i], "i = %lu\n%s", i, status_str.c_str());
  }
}
