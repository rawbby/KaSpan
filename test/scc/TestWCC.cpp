#include <scc/Fuzzy.hpp>
#include <scc/Wcc.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
}
