#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <ispan/scc_detection.hpp>

#include <kamping/communicator.hpp>
#include <mpi.h>

int
main(int argc, char** argv)
{
  if (argc != 2) {
    std::cout << "usage: ./<exe> <kagen_args_string>" << std::endl;
    std::exit(1);
  }

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  comm.barrier();
  double const beg_mpi = MPI_Wtime();
  SCOPE_GUARD(if (comm.is_root()) {
    double const end_mpi = MPI_Wtime();
    std::cout << "Time from MPI_Init() to MPI_Finalize(): " << (end_mpi - beg_mpi) << std::endl;
  });

  double const beg_kagen = MPI_Wtime();
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, argv[1]));
  comm.barrier();
  if (comm.is_root()) {
    double const end_kagen = MPI_Wtime();
    std::cout << "Time KaGen(): " << (end_kagen - beg_kagen) << std::endl;
  }

  ASSERT_TRY(auto const graph, AllGatherGraph(comm, graph_part));

  double const beg_ispan = MPI_Wtime();
  scc_detection(graph, 100, comm.rank(), comm.size());
  comm.barrier();
  if (comm.is_root()) {
    double const end_ispan = MPI_Wtime();
    std::cout << "Time scc_detection(): " << (end_ispan - beg_ispan) << std::endl;
  }
}
