#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <ispan/scc_detection.hpp>

#include <kamping/communicator.hpp>
#include <kamping/measurements/printer.hpp>
#include <kamping/measurements/timer.hpp>
#include <mpi.h>

#include <fstream>

[[noreturn]] void
usage()
{
  std::cout << "usage: ./<exe> --kagen_option_string <kagen_option_string> --output_file <output_file>" << std::endl;
  std::exit(1);
}

std::string
select_kagen_option_string(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--kagen_option_string") == 0)
      return argv[i + 1];
  usage();
}

std::string
select_output_file(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--output_file") == 0)
      return argv[i + 1];
  usage();
}

unsigned long long
select_alpha(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--alpha") == 0)
      return std::stoull(argv[i + 1]);
  return 14;
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);
  auto const alpha               = select_alpha(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  comm.barrier();
  KASPAN_TIME_START("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  KASPAN_TIME_STOP();

  KASPAN_STATISTIC_PUSH("world_rank", std::to_string(comm.rank()));
  KASPAN_STATISTIC_PUSH("world_size", std::to_string(comm.size()));
  KASPAN_STATISTIC_PUSH("kagen_option_string", std::string{ kagen_option_string });

  std::vector<vertex_t> scc_id;

  KASPAN_TIME_START("preprocessing");
  ASSERT_TRY(auto const graph, AllGatherGraph(comm, graph_part));
  KASPAN_TIME_STOP()

  KASPAN_TIME_START("scc");
  scc_detection(graph, alpha, comm.rank(), comm.size(), &scc_id);
  KASPAN_TIME_STOP()

  IF(KASPAN_STATISTIC, size_t global_component_count = 0;)
  for (vertex_t u = 0; u < graph_part.n; ++u)
    if (scc_id[u] == u)
      IF(KASPAN_STATISTIC, ++global_component_count;)
  KASPAN_STATISTIC_PUSH("scc_count", std::to_string(global_component_count));

  std::ofstream output_fd{ output_file };
  kamping::measurements::timer().aggregate_and_print(
    kamping::measurements::SimpleJsonPrinter
      IF(KASPAN_STATISTIC, (output_fd, g_kaspan_statistic), (output_fd)));
}
