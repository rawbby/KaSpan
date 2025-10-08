#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <scc/SCC.hpp>

#include <kamping/communicator.hpp>
#include <kamping/measurements/printer.hpp>
#include <kamping/measurements/timer.hpp>
#include <mpi.h>

#include <fstream>

[[noreturn]] void
usage()
{
  std::cout << "usage: ./<exe> --kagen_option_string <kagen_option_string> --output-file <output_file>" << std::endl;
  std::exit(1);
}

char const*
select_kagen_option_string(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--kagen_option_string") == 0)
      return argv[i + 1];
  usage();
}

char const*
select_output_file(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--output-file") == 0)
      return argv[i + 1];
  usage();
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);

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
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string));
  comm.barrier();
  if (comm.is_root()) {
    double const end_kagen = MPI_Wtime();
    std::cout << "Time KaGen(): " << (end_kagen - beg_kagen) << std::endl;
  }

  ASSERT_TRY(auto scc_id, U64Buffer::create(graph_part.part.n));

  double const beg_kaspan = MPI_Wtime();
  scc_detection(comm, graph_part, scc_id);
  comm.barrier();
  if (comm.is_root()) {
    double const end_kaspan = MPI_Wtime();
    std::cout << "Time scc_detection(): " << (end_kaspan - beg_kaspan) << std::endl;
  }

  std::ofstream output_fd{ output_file };

  std::vector<std::pair<std::string, std::string>> config{
    { "world_rank", std::to_string(comm.rank()) },
    { "world_size", std::to_string(comm.size()) },
    { "kagen_option_string", std::string{ kagen_option_string } }
  };

  kamping::measurements::timer().aggregate_and_print(kamping::measurements::SimpleJsonPrinter{ output_fd, config });
}
