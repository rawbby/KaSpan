#include <scc/Fuzzy.hpp>
#include <wcc/WCC.hpp>
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
}
