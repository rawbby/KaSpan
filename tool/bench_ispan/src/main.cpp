#include "scc/allgather_graph.hpp"

#include <ispan/scc.hpp>
#include <scc/adapter/kagen.hpp>
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
    << std::endl;
  std::exit(1);
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = arg_select_str(argc, argv, "--kagen_option_string", usage);
  auto const output_file         = arg_select_str(argc, argv, "--output_file", usage);
  auto const alpha               = arg_select_default_int<i32>(argc, argv, "--alpha", 14);

  MPI_DEFAULT_INIT();

  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));
  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  MPI_Barrier(MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("world_rank", mpi_world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_world_size);
  KASPAN_STATISTIC_ADD("alpha", alpha);

  KASPAN_STATISTIC_PUSH("kagen");
  auto const graph_part = kagen_graph_part(kagen_option_string);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("preprocessing");
  auto const graph = allgather_graph(
    graph_part.part,
    graph_part.m,
    graph_part.local_fw_m,
    graph_part.fw_head,
    graph_part.fw_csr);
  KASPAN_STATISTIC_POP();

  std::vector<vertex_t> scc_id;
  scc(graph, alpha, mpi_world_rank, mpi_world_size, &scc_id);

  IF_KASPAN_STATISTIC(size_t global_component_count = 0;)
  for (vertex_t u = 0; u < graph.n; ++u)
    if (scc_id[u] == u)
      IF_KASPAN_STATISTIC(++global_component_count;)
  KASPAN_STATISTIC_ADD("scc_count", global_component_count);
}
