#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/async/scc.hpp>
#include <kaspan/scc/scc.hpp>
#include <kaspan/scc/variant/scc_hpc_like.hpp>
#include <kaspan/scc/variant/scc_hpc_trim_ex.hpp>
#include <kaspan/scc/variant/scc_ispan_like.hpp>
#include <kaspan/scc/variant/scc_trim_ex.hpp>
#include <kaspan/scc/variant/scc_trim_ex_light_residual.hpp>
#include <kaspan/scc/variant/scc_trim_ex_residual.hpp>
#include <kaspan/util/arg_parse.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/noop_indirection.hpp>

#include <mpi.h>

#include <cstdio>
#include <limits>
#include <print>

using namespace kaspan;

void
usage(
  int /* argc */,
  char** argv)
{
  std::println("usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file <output_file> [--variant (async | async_indirect | "
               "hpc_like | hpc_trim_ex | ispan_like | trim_ex | trim_ex_light_residual | trim_ex_residual)]",
               argv[0]);
}

template<part_view_concept Part>
void
benchmark(
  bidi_graph_part_view<Part> graph,
  char const*                variant,
  int                        argc,
  char**                     argv)
{
  KASPAN_STATISTIC_ADD("n", graph.part.n());
  KASPAN_STATISTIC_ADD("local_n", graph.part.local_n());
  KASPAN_STATISTIC_ADD("m", mpi_basic::allreduce_single(graph.local_fw_m, mpi_basic::sum));
  KASPAN_STATISTIC_ADD("local_fw_m", graph.local_fw_m);
  KASPAN_STATISTIC_ADD("local_bw_m", graph.local_bw_m);

  // pre-allocate scc_id buffer
  auto const local_n = graph.part.local_n();
  auto       scc_id  = make_array<vertex_t>(local_n);

  mpi_basic::barrier();

  KASPAN_CALLGRIND_START_INSTRUMENTATION();
  if (variant == nullptr) scc(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "async")) async::scc<briefkasten::NoopIndirectionScheme>(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "async_indirect")) async::scc<briefkasten::GridIndirectionScheme>(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "hpc_like")) scc_hpc_like(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "hpc_trim_ex")) scc_hpc_trim_ex(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "ispan_like")) scc_ispan_like(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "trim_ex")) scc_trim_ex(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "trim_ex_light_residual")) scc_trim_ex_light_residual(graph, scc_id.data());
  else if (0 == std::strcmp(variant, "trim_ex_residual")) scc_trim_ex_residual(graph, scc_id.data());
  else usage(argc, argv);
  KASPAN_CALLGRIND_STOP_INSTRUMENTATION();

#if KASPAN_STATISTIC
  vertex_t local_component_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == graph.part.to_global(k)) {
      ++local_component_count;
    }
  }

  auto const global_component_count = mpi_basic::allreduce_single(local_component_count, MPI_SUM);
  KASPAN_STATISTIC_ADD("local_component_count", local_component_count);
  KASPAN_STATISTIC_ADD("global_component_count", global_component_count);
#endif
}

int
main(
  int    argc,
  char** argv)
{
  auto const* const kagen_option_string = arg_select_optional_str(argc, argv, "--kagen_option_string");
  auto const* const manifest_file       = arg_select_optional_str(argc, argv, "--manifest_file");
  if ((kagen_option_string == nullptr ^ manifest_file == nullptr) == 0) {
    usage(argc, argv);
    std::exit(1);
  }

  auto const* const output_file = arg_select_str(argc, argv, "--output_file", usage);
  auto const*       variant     = arg_select_optional_str(argc, argv, "--variant");

  KASPAN_DEFAULT_INIT();
  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));

  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_ADD("world_rank", mpi_basic::world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_basic::world_size);
  KASPAN_STATISTIC_ADD("memcheck", KASPAN_MEMCHECK);
  KASPAN_STATISTIC_ADD("callgrind", KASPAN_CALLGRIND);

  if (kagen_option_string != nullptr) {
    KASPAN_STATISTIC_PUSH("kagen");
    auto const kagen_graph = kagen_graph_part(kagen_option_string);
    KASPAN_STATISTIC_POP();
    benchmark(kagen_graph.view(), variant, argc, argv);
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest = manifest::load(manifest_file);
    ASSERT_LT(manifest.graph_node_count, std::numeric_limits<vertex_t>::max());
    auto const part           = balanced_slice_part{ integral_cast<vertex_t>(manifest.graph_node_count) };
    auto const manifest_graph = load_graph_part_from_manifest(part, manifest);
    KASPAN_STATISTIC_POP();
    benchmark(manifest_graph.view(), variant, argc, argv);
  }
}
