#include <graph/Graph.hpp>
#include <ispan/scc_detection.hpp>

#include <mpi.h>

int
main(int args, char** argv)
{
  if (args != 11) {
    std::cout << "Usage: ./scc_cpu <manifest> <alpha>\n";
    exit(-1);
  }

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  int world_size;
  int world_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  int const alpha = atof(argv[2]);

  ASSERT_TRY(auto const manifest, Manifest::load(argv[1]));
  ASSERT_TRY(auto const graph, Graph::load(manifest));

  scc_detection(graph, alpha, world_rank, world_size);
  MPI_Finalize();
}
