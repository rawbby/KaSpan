#include <ispan/scc.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/scc/fuzzy.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <iomanip>
#include <iostream>

using namespace kaspan;

auto
main(int argc, char** argv) -> int
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  for (vertex_t n = 32; n < 64; n += 8) {

    auto const s = mpi_basic::allreduce_max_time();
    auto const [scc_id_orig, g] = fuzzy_global_scc_id_and_graph(s, n);

    std::vector<vertex_t> scc_id_ispan;
    scc(g.n, g.m, g.fw.head, g.fw.csr, g.bw.head, g.bw.csr, 16, &scc_id_ispan);

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
      ASSERT_EQ(scc_id_orig[i], scc_id_ispan[i], "i={}\n{}", i, status_str.c_str());
  }
}
