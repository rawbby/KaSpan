#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>
#include <scc/Scc.hpp>
#include <scc/async/Scc.hpp>
#include <debug/Statistic.hpp>

#include <kamping/communicator.hpp>
#include <kamping/measurements/printer.hpp>

#include <mpi.h>

#include <fstream>

[[noreturn]] void
usage()
{
  std::cout << "usage: ./<exe> --kagen_option_string <kagen_option_string> --output_file <output_file> [--async [--async_indirect]]"
            << std::endl;
  std::exit(1);
}

auto
select_async_indirect(int argc, char** argv) -> bool
{
  for (int j = 1; j < argc - 1; ++j) {
    if (strcmp(argv[j], "--async_indirect") == 0) {
      return true;
    }
  }
  return false;
}

auto
select_async(int argc, char** argv) -> bool
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], "--async") == 0) {
      return true;
    }
  }
  return false;
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
  auto const async               = select_async(argc, argv);
  auto const async_indirect      = select_async_indirect(argc, argv);

  if (async_indirect and not async)
    usage();

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file.c_str()));
  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  auto comm = kamping::Communicator{};
  comm.barrier();

  KASPAN_STATISTIC_ADD("world_rank", comm.rank());
  KASPAN_STATISTIC_ADD("world_size", comm.size());
  KASPAN_STATISTIC_ADD("async", async);
  KASPAN_STATISTIC_ADD("async_indirect", async_indirect);

  KASPAN_STATISTIC_PUSH("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  KASPAN_STATISTIC_POP();

  ASSERT_TRY(auto scc_id, U64Buffer::create(graph_part.part.n));

  if (not async) {
    scc(comm, graph_part, scc_id);
  } else if (async_indirect) {
    async::scc<briefkasten::GridIndirectionScheme>(comm, graph_part, scc_id);
  } else {
    async::scc<briefkasten::NoopIndirectionScheme>(comm, graph_part, scc_id);
  }

#if KASPAN_STATISTIC
  size_t local_component_count = 0;
  for (vertex_t k = 0; k < graph_part.part.size(); ++k)
    if (scc_id.get(k) == graph_part.part.select(k))
      ++local_component_count;

  auto const global_component_count = comm.allreduce_single(kamping::send_buf(local_component_count), kamping::op(MPI_SUM));
  KASPAN_STATISTIC_ADD("local_component_count", local_component_count);
  KASPAN_STATISTIC_ADD("global_component_count", global_component_count);
#endif
}
