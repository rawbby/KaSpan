#include "distributed/Graph.hpp"
#include "wtime.h"

#include "scc_common.h"
#include <iostream>
#include <mpi.h>
#include <vector>

VoidResult
run(int args, char** argv)
{
  using namespace distributed;

  if (args != 8) {
    std::cout << "Usage: ./scc_cpu <manifest> <thread_count> <alpha> <beta> <gamma> <theta> <run_times>\n";
    exit(-1);
  }

  MPI_Init(nullptr, nullptr);

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  RESULT_TRY(const auto manifest, Manifest::load(argv[1]));
  const auto partition = TrivialSlicePartition{
    manifest.graph_node_count,
    static_cast<std::size_t>(world_rank),
    static_cast<std::size_t>(world_size)
  };

  const index_t thread_count = world_size;

  const int     alpha     = atof(argv[3]);
  const index_t run_times = atoi(argv[7]);

  std::vector<double> avg_time(15);

  RESULT_TRY(const auto graph, Graph<>::create(manifest, partition));

  // graph* g = graph_load(fw_beg_file,
  //                       fw_csr_file,
  //                       bw_beg_file,
  //                       bw_csr_file,
  //                       avg_time);
  index_t i = 0;

  while (i++ < run_times) {
    std::vector<vertex_t> empty{}; // todo get rid of this
    printf("\nRuntime: %lu\n", i);
    scc_detection(g,
                  alpha,
                  thread_count,
                  avg_time,
                  world_rank,
                  world_size,
                  i,
                  empty);
  }

  if (world_rank == 0)
    print_time_result(run_times - 1,
                      avg_time);

  MPI_Finalize();

  return VoidResult::success();
}

int
main(int argc, char** argv)
{
  return static_cast<int>(run(argc, argv).error_or_ok());
}
