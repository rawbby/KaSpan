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
  bool indirect = false;
};

[[noreturn]] void
usage()
{
  std::cout << "usage: ./<exe> --kagen_option_string <kagen_option_string> --output_file <output_file> [--async [--async_indirect]]" << std::endl;
  std::exit(1);
}

CommStrategy
select_comm_strategy(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], "--async") == 0) {
      bool indirect = false;
      for (int j = 1; j < argc - 1; ++j) {
        if (strcmp(argv[j], "--async_indirect") == 0) {
          indirect = true;
          break;
        }
      }
      if (indirect)
        return { true, true };
      return { true, false };
    }
  }
  return { false, false };
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

int
main(int argc, char** argv)
{
  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);
  auto const comm_strategy       = select_comm_strategy(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  comm.barrier();
  KASPAN_TIME_START("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  KASPAN_TIME_STOP();

  KASPAN_STATISTIC_PUSH("world_rank", std::to_string(comm.rank()));
  KASPAN_STATISTIC_PUSH("world_size", std::to_string(comm.size()));
  KASPAN_STATISTIC_PUSH("async", std::to_string(comm_strategy.async));
  KASPAN_STATISTIC_PUSH("async_indirect", std::to_string(comm_strategy.indirect));
  KASPAN_STATISTIC_PUSH("kagen_option_string", std::string{ kagen_option_string });

  ASSERT_TRY(auto scc_id, U64Buffer::create(graph_part.part.n));

  KASPAN_TIME_START("scc");
  if (not comm_strategy.async)
    sync_scc_detection(comm, graph_part, scc_id);
  else if (comm_strategy.indirect)
    async_scc_detection<briefkasten::GridIndirectionScheme>(comm, graph_part, scc_id);
  else
    async_scc_detection<briefkasten::NoopIndirectionScheme>(comm, graph_part, scc_id);
  KASPAN_TIME_STOP()

  IF(KASPAN_STATISTIC, size_t component_count = 0;)
  for (vertex_t k = 0; k < graph_part.part.size(); ++k)
    if (scc_id.get(k) == graph_part.part.select(k))
      IF(KASPAN_STATISTIC, ++component_count;)

  IF(KASPAN_STATISTIC, size_t global_component_count = comm.allreduce_single(kamping::send_buf(component_count), kamping::op(MPI_SUM));)
  KASPAN_STATISTIC_PUSH("scc_count", std::to_string(global_component_count));

  std::ofstream output_fd{ output_file };
  kamping::measurements::timer().aggregate_and_print(
    kamping::measurements::SimpleJsonPrinter
      IF(KASPAN_STATISTIC, (output_fd, g_kaspan_statistic), (output_fd)));
}
