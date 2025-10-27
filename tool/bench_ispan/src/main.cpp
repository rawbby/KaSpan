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
  kaspan_statistic_push("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  kaspan_statistic_pop();

  kaspan_statistic_add("world_rank", comm.rank());
  kaspan_statistic_add("world_size", comm.size());

  std::vector<vertex_t> scc_id;

  kaspan_statistic_push("preprocessing");
  ASSERT_TRY(auto const graph, AllGatherGraph(comm, graph_part));
  kaspan_statistic_pop();

  kaspan_statistic_push("scc");
  scc_detection(graph, alpha, comm.rank(), comm.size(), &scc_id);
  kaspan_statistic_pop();

  IF(KASPAN_STATISTIC, size_t global_component_count = 0;)
  for (vertex_t u = 0; u < graph_part.n; ++u)
    if (scc_id[u] == u)
      IF(KASPAN_STATISTIC, ++global_component_count;)
  kaspan_statistic_add("scc_count", global_component_count);

  kaspan_statistic_mpi_write_json(output_file.c_str());
}
