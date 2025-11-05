#include <../../core/essential/include/debug/SubProcess.hpp>
#include <scc/Fuzzy.hpp>
#include <scc/Wcc.hpp>
#include <util/scope_guard.hpp>

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());
}
