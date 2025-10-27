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
  KASPAN_STATISTIC_SCOPE("benchmark");

  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);
  auto const comm_strategy       = select_comm_strategy(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  auto comm = kamping::Communicator{};
  comm.barrier();

  kaspan_statistic_push("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  kaspan_statistic_pop();

  kaspan_statistic_add("world_rank", comm.rank());
  kaspan_statistic_add("world_size", comm.size());
  kaspan_statistic_add("async", comm_strategy.async);
  kaspan_statistic_add("async_indirect", comm_strategy.indirect);

  ASSERT_TRY(auto scc_id, U64Buffer::create(graph_part.part.n));

  kaspan_statistic_push("scc");
  if (not comm_strategy.async)
    sync_scc_detection(comm, graph_part, scc_id);
  else if (comm_strategy.indirect)
    async_scc_detection<briefkasten::GridIndirectionScheme>(comm, graph_part, scc_id);
  else
    async_scc_detection<briefkasten::NoopIndirectionScheme>(comm, graph_part, scc_id);
  kaspan_statistic_pop();

  IF(KASPAN_STATISTIC, size_t component_count = 0;)
  for (vertex_t k = 0; k < graph_part.part.size(); ++k)
    if (scc_id.get(k) == graph_part.part.select(k))
      IF(KASPAN_STATISTIC, ++component_count;)

  IF(KASPAN_STATISTIC, size_t global_component_count = comm.allreduce_single(kamping::send_buf(component_count), kamping::op(MPI_SUM));)
  kaspan_statistic_add("local_scc_count", component_count);
  kaspan_statistic_add("global_scc_count", global_component_count);

  if (comm.is_root()) {
    std::ofstream os{ output_file };
    os << '{' << '"' << comm.rank() << '"' << ':';
    kaspan_statistic_write_json(os);

    std::vector<size_t> counts(comm.size(), 0);

    MPI_Gather(
      nullptr, 0, mpi_basic_type<size_t>, &counts[1], comm.size() - 1, mpi_basic_type<size_t>, comm.root_signed(), MPI_COMM_WORLD);

  } else {
    std::stringstream ss;
    kaspan_statistic_write_json(ss);
    auto const json = ss.str();

    auto const   buffer      = std::as_bytes(std::span{ json.cbegin(), json.cend() });
    size_t const buffer_size = buffer.size();

    MPI_Gather(
      &buffer_size, 1, mpi_basic_type<size_t>, nullptr, 0, mpi_basic_type<size_t>, comm.root_signed(), MPI_COMM_WORLD);
  }
}
