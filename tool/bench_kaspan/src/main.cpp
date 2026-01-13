#include <kaspan/debug/assert_lt.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/allreduce_single.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/async/scc.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/scc.hpp>
#include <kaspan/util/arg_parse.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/noop_indirection.hpp>

#include <mpi.h>

#include <cstdio>
#include <limits>
#include <print>

using namespace kaspan;

void
usage(int /* argc */, char** argv)
{
  std::println("usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file <output_file> [--async [--async_indirect]] [--trim_tarjan]",
               argv[0]);
}

void
benchmark(auto const& graph, bool use_async, bool use_async_indirect)
{
  KASPAN_STATISTIC_ADD("async", use_async);
  KASPAN_STATISTIC_ADD("async_indirect", use_async_indirect);

  KASPAN_STATISTIC_ADD("n", graph.part.n);
  KASPAN_STATISTIC_ADD("local_n", graph.part.local_n());
  KASPAN_STATISTIC_ADD("m", graph.m);
  KASPAN_STATISTIC_ADD("local_fw_m", graph.local_fw_m);
  KASPAN_STATISTIC_ADD("local_bw_m", graph.local_bw_m);

  // pre-allocate scc_id buffer
  auto  scc_id_buffer = make_buffer<vertex_t>(graph.part.local_n());
  auto* scc_id_access = scc_id_buffer.data();
  auto* scc_id        = borrow_array_clean<vertex_t>(&scc_id_access, graph.part.local_n());

  MPI_Barrier(MPI_COMM_WORLD);
  if (not use_async) {
    KASPAN_CALLGRIND_START_INSTRUMENTATION();
    scc(graph.part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);
    KASPAN_CALLGRIND_STOP_INSTRUMENTATION();
  } else if (use_async_indirect) {
    KASPAN_CALLGRIND_START_INSTRUMENTATION();
    async::scc<briefkasten::GridIndirectionScheme>(graph.part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);
    KASPAN_CALLGRIND_STOP_INSTRUMENTATION();
  } else {
    KASPAN_CALLGRIND_START_INSTRUMENTATION();
    async::scc<briefkasten::NoopIndirectionScheme>(graph.part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);
    KASPAN_CALLGRIND_STOP_INSTRUMENTATION();
  }

#if KASPAN_STATISTIC
  vertex_t local_component_count = 0;
  for (vertex_t k = 0; k < graph.part.local_n(); ++k) {
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
main(int argc, char** argv)
{
  auto const* const kagen_option_string = arg_select_optional_str(argc, argv, "--kagen_option_string");
  auto const* const manifest_file       = arg_select_optional_str(argc, argv, "--manifest_file");
  if ((kagen_option_string == nullptr ^ manifest_file == nullptr) == 0) {
    usage(argc, argv);
    std::exit(1);
  }

  auto const* const output_file = arg_select_str(argc, argv, "--output_file", usage);

  auto const use_async          = arg_select_flag(argc, argv, "--async");
  auto const use_async_indirect = arg_select_flag(argc, argv, "--async_indirect");
  if (use_async_indirect and not use_async) {
    usage(argc, argv);
  }

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
    benchmark(kagen_graph, use_async, use_async_indirect);
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest = manifest::load(manifest_file);
    ASSERT_LT(manifest.graph_node_count, std::numeric_limits<vertex_t>::max());
    auto const part = balanced_slice_part{ integral_cast<vertex_t>(manifest.graph_node_count) };
    {
      integral_cast<vertex_t>(manifest.graph_node_count);
    };
    auto const manifest_graph = load_graph_part_from_manifest(part, manifest);
    KASPAN_STATISTIC_POP();
    benchmark(manifest_graph, use_async, use_async_indirect);
  }
}
