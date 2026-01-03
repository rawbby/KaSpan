#include <cstdlib>
#include <ispan/scc.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/allgather_graph.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/util/arg_parse.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <mpi.h>

#include <cstdio>
#include <print>

void
usage(int /* argc */, char** argv)
{
  std::println("usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file <output_file>", argv[0]);
}

void
benchmark(auto const& graph, kaspan::i32 alpha)
{
  std::vector<kaspan::vertex_t> scc_id;

  scc(graph.n, graph.m, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, alpha, &scc_id);

  IF_KASPAN_STATISTIC(size_t global_component_count = 0;)
  for (kaspan::vertex_t u = 0; u < graph.n; ++u) {
    if (scc_id[u] == u) {
      IF_KASPAN_STATISTIC(++global_component_count;)
    }
  }
  KASPAN_STATISTIC_ADD("scc_count", global_component_count);
}

int
main(int argc, char** argv)
{
  auto const* const kagen_option_string = kaspan::arg_select_optional_str(argc, argv, "--kagen_option_string");
  auto const* const manifest_file       = kaspan::arg_select_optional_str(argc, argv, "--manifest_file");
  if ((kagen_option_string == nullptr ^ manifest_file == nullptr) == 0) {
    usage(argc, argv);
    std::exit(1);
  }

  auto const* const output_file = kaspan::arg_select_str(argc, argv, "--output_file", usage);
  auto const        alpha       = kaspan::arg_select_default_int<kaspan::i32>(argc, argv, "--alpha", 14);

  KASPAN_DEFAULT_INIT();

  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));
  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", kaspan::get_resident_set_bytes());

  MPI_Barrier(MPI_COMM_WORLD);

  KASPAN_STATISTIC_ADD("world_rank", kaspan::mpi_basic::world_rank);
  KASPAN_STATISTIC_ADD("world_size", kaspan::mpi_basic::world_size);
  KASPAN_STATISTIC_ADD("valgrind", kaspan::KASPAN_VALGRIND_RUNNING_ON_VALGRIND);
  KASPAN_STATISTIC_ADD("alpha", alpha);

  if (kagen_option_string != nullptr) {
    KASPAN_STATISTIC_PUSH("kagen");
    auto const graph_part = kaspan::kagen_graph_part(kagen_option_string);
    KASPAN_STATISTIC_POP();
    KASPAN_STATISTIC_PUSH("preprocessing");
    auto const graph = allgather_graph(graph_part.part, graph_part.m, graph_part.local_fw_m, graph_part.fw_head, graph_part.fw_csr);
    KASPAN_STATISTIC_POP();
    benchmark(graph, alpha);
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest       = kaspan::manifest::load(manifest_file);
    auto const manifest_graph = kaspan::load_graph_from_manifest(manifest);
    KASPAN_STATISTIC_POP();
    benchmark(manifest_graph, alpha);
  }
}
