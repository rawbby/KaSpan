#include "util/arithmetic.hpp"
#include "scc/base.hpp"
#include "debug/statistic.hpp"
#include <cstdlib>
#include "util/scope_guard.hpp"
#include "debug/process.hpp"
#include "mpi_basic/world.hpp"
#include <debug/valgrind.hpp>
#include <ispan/scc.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/adapter/manifest.hpp>
#include <scc/allgather_graph.hpp>
#include <util/arg_parse.hpp>

#include <mpi.h>

#include <cstdio>
#include <print>
#include <vector>

void
usage(int /* argc */, char** argv)
{
  std::println("usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file <output_file>", argv[0]);
}

void
benchmark(auto const& graph, i32 alpha)
{
  std::vector<vertex_t> scc_id;

  scc(graph.n, graph.m, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, alpha, &scc_id);

  IF_KASPAN_STATISTIC(size_t global_component_count = 0;)
  for (vertex_t u = 0; u < graph.n; ++u) {
    if (scc_id[u] == u) { IF_KASPAN_STATISTIC(++global_component_count;) }
  }
  KASPAN_STATISTIC_ADD("scc_count", global_component_count);
}

int
main(int argc, char** argv)
{
  const auto *const kagen_option_string = arg_select_optional_str(argc, argv, "--kagen_option_string");
  const auto *const manifest_file       = arg_select_optional_str(argc, argv, "--manifest_file");
  if ((kagen_option_string == nullptr ^ manifest_file == nullptr) == 0) {
    usage(argc, argv);
    std::exit(1);
  }

  const auto *const output_file = arg_select_str(argc, argv, "--output_file", usage);
  auto const alpha       = arg_select_default_int<i32>(argc, argv, "--alpha", 14);

  KASPAN_DEFAULT_INIT();

  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));
  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  MPI_Barrier(MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("world_rank", mpi_basic::world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_basic::world_size);
  KASPAN_STATISTIC_ADD("valgrind", KASPAN_VALGRIND_RUNNING_ON_VALGRIND);
  KASPAN_STATISTIC_ADD("alpha", alpha);

  if (kagen_option_string != nullptr) {
    KASPAN_STATISTIC_PUSH("kagen");
    auto const graph_part = kagen_graph_part(kagen_option_string);
    KASPAN_STATISTIC_POP();
    KASPAN_STATISTIC_PUSH("preprocessing");
    auto const graph = allgather_graph(graph_part.part, graph_part.m, graph_part.local_fw_m, graph_part.fw_head, graph_part.fw_csr);
    KASPAN_STATISTIC_POP();
    benchmark(graph, alpha);
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest       = Manifest::load(manifest_file);
    auto const manifest_graph = load_graph_from_manifest(manifest);
    KASPAN_STATISTIC_POP();
    benchmark(manifest_graph, alpha);
  }
}
