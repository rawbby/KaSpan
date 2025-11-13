#include <debug/statistic.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/backward_complement.hpp>
#include <scc/scc.hpp>
#include <util/arg_parse.hpp>

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
    << " [--trim_tarjan]"
    << std::endl;
  std::exit(1);
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = arg_select_str(argc, argv, "--kagen_option_string", usage);
  auto const output_file         = arg_select_str(argc, argv, "--output_file", usage);
  auto const use_async           = arg_select_flag(argc, argv, "--async");
  auto const use_async_indirect  = arg_select_flag(argc, argv, "--async_indirect");
  auto const use_trim_tarjan     = arg_select_flag(argc, argv, "--trim_tarjan");

  if (use_async_indirect and not use_async)
    usage(argc, argv);

  KASPAN_DEFAULT_INIT();
  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));

  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  MPI_Barrier(MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("world_rank", mpi_world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_world_size);
  KASPAN_STATISTIC_ADD("async", use_async);
  KASPAN_STATISTIC_ADD("async_indirect", use_async_indirect);
  KASPAN_STATISTIC_ADD("trim_tarjan", use_trim_tarjan);

  // generate kagen graph (including backward graph requiring communication)
  KASPAN_STATISTIC_PUSH("kagen");
  auto const kagen_graph = kagen_graph_part(kagen_option_string);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_ADD("n", kagen_graph.part.n);
  KASPAN_STATISTIC_ADD("local_n", kagen_graph.part.local_n());
  KASPAN_STATISTIC_ADD("m", kagen_graph.m);
  KASPAN_STATISTIC_ADD("local_fw_m", kagen_graph.local_fw_m);
  KASPAN_STATISTIC_ADD("local_bw_m", kagen_graph.local_bw_m);

  // pre-allocate scc_id buffer
  auto  scc_id_buffer = Buffer::create<vertex_t>(kagen_graph.part.local_n());
  auto* scc_id_access = scc_id_buffer.data();
  auto* scc_id        = borrow_clean<vertex_t>(scc_id_access, kagen_graph.part.local_n());

  if (not use_async) {

    if (use_trim_tarjan) {
      scc<true>(kagen_graph.part, kagen_graph.fw_head, kagen_graph.fw_csr, kagen_graph.bw_head, kagen_graph.bw_csr, scc_id);
    } else {
      scc<false>(kagen_graph.part, kagen_graph.fw_head, kagen_graph.fw_csr, kagen_graph.bw_head, kagen_graph.bw_csr, scc_id);
    }
  }

  // else if (use_async_indirect) {
  //   async::scc<briefkasten::GridIndirectionScheme>(kagen_graph, scc_id);
  // } else {
  //   async::scc<briefkasten::NoopIndirectionScheme>(kagen_graph, scc_id);
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
