#include <scc/Fuzzy.hpp>
#include <scc/SCC.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Time.hpp>
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

  auto comm = kamping::Communicator{};

  for (size_t n = 32; n < 64; n += 8) {

    auto const seed = std::bit_cast<u64>(mpi_global_max_wtime());
    auto const part = BalancedSlicePartition{ n, comm.rank(), comm.size() };

    std::cout << "[Test]\n";
    std::cout << "  Beginning random instance creation ..." << std::endl;

    auto const [scc_id_orig, graph] = [&] {
      auto result = fuzzy_local_scc_id_and_graph(seed, part);
      ASSERT(result.has_value());
      return std::move(result.value());
    }();

    std::cout << "[Test]\n";
    std::cout << "  Finished random instance creation\n";
    std::cout << "  Beginning scc detection ..." << std::endl;

    auto scc_id_ispan = [&] {
      auto result = U64Buffer::create(n);
      ASSERT(result.has_value());
      return std::move(result.value());
    }();

    scc_detection(comm, graph, scc_id_ispan);

    std::cout << "[Test]\n";
    std::cout << "  Finished scc detection" << std::endl;

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
