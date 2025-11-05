#include <../../../core/essential/include/debug/assert.hpp>
#include <../../../core/essential/include/debug/sub_process.hpp>
#include <ispan/scc.hpp>
#include <scc/Fuzzy.hpp>
#include <util/Time.hpp>
#include <util/Util.hpp>
#include <util/scope_guard.hpp>

#include <iomanip>
#include <iostream>

auto
main(int argc, char** argv) -> int
{
  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  for (size_t n = 32; n < 64; n += 8) {

    auto const seed = std::bit_cast<u64>(mpi_global_max_wtime());
    auto const part = BalancedSlicePart{ n, comm.rank(), comm.size() };

    ASSERT_TRY(auto scc_id_and_graph, fuzzy_global_scc_id_and_graph(seed, part.n));
    auto const scc_id_orig = std::move(scc_id_and_graph.first);
    auto const graph       = std::move(scc_id_and_graph.second);

    std::vector<vertex_t> scc_id_ispan;
    scc(graph, 16, comm.rank(), comm.size(), &scc_id_ispan);

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
