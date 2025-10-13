#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <scc/SCC.hpp>

#include <kamping/communicator.hpp>
#include <kamping/measurements/printer.hpp>
#include <kamping/measurements/timer.hpp>
#include <mpi.h>

#include <fstream>

struct CommStrategy
{
  bool async    = false;
  bool buffered = false;
  bool indirect = false;
};

[[noreturn]] void
usage()
{
  std::cout << "usage: ./<exe> --kagen_option_string <kagen_option_string> --output_file <output_file> [--async [--async_buffered] [--async_indirect]]" << std::endl;
  std::exit(1);
}

CommStrategy
select_comm_strategy(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], "--async") == 0) {

      bool buffered = false;
      bool indirect = false;

      for (int j = 1; j < argc - 1; ++j) {
        if (strcmp(argv[j], "--async_buffered") == 0) {
          buffered = true;
          break;
        }
      }

      for (int j = 1; j < argc - 1; ++j) {
        if (strcmp(argv[j], "--async_indirect") == 0) {
          indirect = true;
          break;
        }
      }

      if (buffered and indirect)
        return { true, true, true };
      if (buffered)
        return { true, true, false };
      if (indirect)
        return { true, false, true };

      return { true };
    }
  }
  return {};
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
      return std::string{ argv[i + 1] } + ".json";
  usage();
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);
  auto const comm_strategy       = select_comm_strategy(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  kamping::measurements::timer().synchronize_and_start("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  kamping::measurements::timer().stop();

  ASSERT_TRY(auto scc_id, U64Buffer::create(graph_part.part.n));

  kamping::measurements::timer().synchronize_and_start("kaspan");
  kamping::measurements::timer().synchronize_and_start("kaspan_scc");

  if (not comm_strategy.async)
    scc_detection(comm, graph_part, scc_id);
  else if (comm_strategy.buffered and comm_strategy.indirect)
    scc_detection<true, true, true>(comm, graph_part, scc_id);
  else if (comm_strategy.buffered)
    scc_detection<true, true, false>(comm, graph_part, scc_id);
  else if (comm_strategy.indirect)
    scc_detection<true, false, true>(comm, graph_part, scc_id);
  else
    scc_detection<true, false, false>(comm, graph_part, scc_id);

  kamping::measurements::timer().stop();
  kamping::measurements::timer().stop();

  std::ofstream output_fd{ output_file };

  std::vector<std::pair<std::string, std::string>> config{
    { "world_rank", std::to_string(comm.rank()) },
    { "world_size", std::to_string(comm.size()) },
    { "async", std::to_string(comm_strategy.async) },
    { "async_buffered", std::to_string(comm_strategy.buffered) },
    { "async_indirect", std::to_string(comm_strategy.indirect) },
    { "kagen_option_string", std::string{ kagen_option_string } }
  };

  kamping::measurements::timer().aggregate_and_print(kamping::measurements::SimpleJsonPrinter{ output_fd, config });
}
