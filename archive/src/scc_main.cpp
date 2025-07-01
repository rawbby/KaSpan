#include "../include/scc/graph.h"
#include "../../src/wtime.h"

#include "../include/scc/scc_common.h"
#include <mpi.h>
#include <vector>
#include <iostream>

int
main(int args, char** argv)
{
  if (args != 11) {
    std::cout << "Usage: ./scc_cpu <fw_beg_file> <fw_csr_file> <bw_beg_file> <bw_csr_file> <thread_count> <alpha> <beta> <gamma> <theta> <run_times>\n";
    exit(-1);
  }

  MPI_Init(nullptr, nullptr);

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  const char* fw_beg_file = argv[1];
  const char* fw_csr_file = argv[2];
  const char* bw_beg_file = argv[3];
  const char* bw_csr_file = argv[4];
  const index_t thread_count = world_size;

  const int alpha = atof(argv[6]);
  const index_t run_times = atoi(argv[10]);

  std::vector<double> avg_time(15);

  graph* g = graph_load(fw_beg_file,
                        fw_csr_file,
                        bw_beg_file,
                        bw_csr_file,
                        avg_time);
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
                  i, empty);
  }

  if (world_rank == 0)
    print_time_result(run_times - 1,
                      avg_time);

  MPI_Finalize();

  return 0;
}
