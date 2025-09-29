#include <scc/Fuzzy.hpp>
#include <scc/SCC.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Time.hpp>
#include <util/Util.hpp>

#include <iomanip>

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};

  for (size_t n = 32; n < 64; n += 8) {

    auto const seed = std::bit_cast<u64>(mpi_global_max_wtime());
    auto const part = BalancedSlicePart{ n, comm.rank(), comm.size() };

    ASSERT_TRY(auto scc_id_and_graph, fuzzy_local_scc_id_and_graph(seed, part));
    auto const [scc_id_orig, graph] = std::move(scc_id_and_graph);

    ASSERT_TRY(auto scc_id_ispan, U64Buffer::zeroes(graph.part.size()));

    scc_detection(comm, graph, scc_id_ispan);

    std::stringstream ss;
    ss << "  index         :";
    for (size_t k = 0; k < graph.part.size(); ++k)
      ss << ' ' << std::right << std::setw(2) << graph.part.select(k);
    ss << "\n  scc_id_orig   :";
    for (size_t k = 0; k < graph.part.size(); ++k)
      ss << ' ' << std::right << std::setw(2) << scc_id_orig[k];
    ss << "\n  scc_id_kaspan :";
    for (size_t k = 0; k < graph.part.size(); ++k)
      ss << ' ' << std::right << std::setw(2) << scc_id_ispan[k];
    ss << "\n                 ";
    for (size_t k = 0; k < graph.part.size(); ++k)
      ss << (scc_id_orig[k] == scc_id_ispan[k] ? "   " : " ^^");
    ss << std::endl;

    auto const status_str = ss.str();

    MPI_Barrier(MPI_COMM_WORLD);

    for (size_t k = 0; k < graph.part.size(); ++k)
      ASSERT_EQ(scc_id_orig[k], scc_id_ispan[k], "k = %lu / i = %lu\n%s", k, graph.part.select(k), status_str.c_str());
  }
}
