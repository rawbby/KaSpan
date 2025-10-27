#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <ispan/scc.hpp>

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
  KASPAN_STATISTIC_PUSH("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);
  auto const alpha               = select_alpha(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  comm.barrier();

  KASPAN_STATISTIC_ADD("world_rank", comm.rank());
  KASPAN_STATISTIC_ADD("world_size", comm.size());
  KASPAN_STATISTIC_ADD("alpha", alpha);

  KASPAN_STATISTIC_PUSH("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("preprocessing");
  ASSERT_TRY(auto const graph, AllGatherGraph(comm, graph_part));
  KASPAN_STATISTIC_POP();

  std::vector<vertex_t> scc_id;
  scc(graph, alpha, comm.rank(), comm.size(), &scc_id);

  IF_KASPAN_STATISTIC(size_t global_component_count = 0;)
  for (vertex_t u = 0; u < graph.n; ++u)
    if (scc_id[u] == u)
      IF_KASPAN_STATISTIC(++global_component_count;)
  KASPAN_STATISTIC_ADD("scc_count", global_component_count);

  KASPAN_STATISTIC_POP();
  KASPAN_STATISTIC_MPI_WRITE_JSON(output_file.c_str());
}
