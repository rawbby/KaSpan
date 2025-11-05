#include <debug/statistic.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/backward_complement.hpp>
#include <scc/scc.hpp>
#include <util/arg_parse.hpp>

// #include <scc/async/Scc.hpp>

#include <mpi.h>

#include <fstream>
#include <iostream>

[[noreturn]] void
usage(int /* argc */, char** argv)
{
  std::cout
    << "usage: " << argv[0]
    << " --kagen_option_string <kagen_option_string>"
    << " --output_file <output_file>"
    << " [--async [--async_indirect]]"
    << std::endl;
  std::exit(1);
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = arg_select_str(argc, argv, "--kagen_option_string", usage);
  auto const output_file         = arg_select_str(argc, argv, "--output_file", usage);
  auto const async               = arg_select_flag(argc, argv, "--async");
  auto const async_indirect      = arg_select_flag(argc, argv, "--async_indirect");

  if (async_indirect and not async)
    usage(argc, argv);

  MPI_DEFAULT_INIT();
  init_mpi_edge_t();
  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));

  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  MPI_Barrier(MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("world_rank", mpi_world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_world_size);
  KASPAN_STATISTIC_ADD("async", async);
  KASPAN_STATISTIC_ADD("async_indirect", async_indirect);

  // generate kagen graph (including backward graph requiring communication)
  KASPAN_STATISTIC_PUSH("kagen");
  auto const kagen_graph = kagen_graph_part(kagen_option_string);
  KASPAN_STATISTIC_POP();

  // pre-allocate scc_id buffer
  auto  scc_id_buffer = Buffer::create<vertex_t>(kagen_graph.part.local_n());
  auto* scc_id_access = scc_id_buffer.data();
  auto* scc_id        = borrow_clean<vertex_t>(scc_id_access, kagen_graph.part.local_n());

  // if (not async) {
  scc(kagen_graph.part, kagen_graph.fw_head, kagen_graph.fw_csr, kagen_graph.bw_head, kagen_graph.bw_csr, scc_id);
  // } else if (async_indirect) {
  //   async::scc<briefkasten::GridIndirectionScheme>(comm, graph_part, scc_id);
  // } else {
  //   async::scc<briefkasten::NoopIndirectionScheme>(comm, graph_part, scc_id);
  // }

#if KASPAN_STATISTIC
  vertex_t local_component_count = 0;
  for (vertex_t k = 0; k < kagen_graph.part.local_n(); ++k)
    if (scc_id[k] == kagen_graph.part.to_global(k))
      ++local_component_count;

  auto const global_component_count = mpi_basic_allreduce_single(local_component_count, MPI_SUM);
  KASPAN_STATISTIC_ADD("local_component_count", local_component_count);
  KASPAN_STATISTIC_ADD("global_component_count", global_component_count);
#endif
}
