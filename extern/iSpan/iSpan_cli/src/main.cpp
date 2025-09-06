#include "graph.h"
#include "scc_common.h"

#include <mpi.h>

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
  char const*   fw_beg_file = argv[1];
  char const*   fw_csr_file = argv[2];
  char const*   bw_beg_file = argv[3];
  char const*   bw_csr_file = argv[4];
  int const     alpha       = atof(argv[6]);
  int const     beta        = atof(argv[7]);
  index_t const run_times   = atoi(argv[10]);
  auto*         avg_time    = new double[15];
  graph*        g           = graph_load(fw_beg_file, fw_csr_file, bw_beg_file, bw_csr_file, avg_time);
  index_t       i           = 0;
  while (i++ < run_times) {
    printf("\nRuntime: %d\n", i);
    scc_detection(g, alpha, beta, avg_time, world_rank, world_size, i);
  }
  if (world_rank == 0)
    print_time_result(run_times - 1, avg_time);
  MPI_Finalize();
}
