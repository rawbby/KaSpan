#include <kaspan/debug/assert_lt.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/mpi_basic/barrier.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/pivot_selection.hpp>
#include <kaspan/util/arg_parse.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/scope_guard.hpp>
#include <kaspan_adapter.hpp>

#include <scc.h>

#include <mpi.h>
#include <omp.h>

#include <cstdio>
#include <limits>
#include <print>

using namespace kaspan;

int  procid, nprocs;
bool verbose = false;
bool debug   = false;
bool verify  = false;
bool output  = false;

void
usage(int /* argc */, char** argv)
{
  std::println("usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file "
               "<output_file> [--threads <threads>]",
               argv[0]);
}

void
benchmark(auto&& graph_part)
{
  using part_t = std::remove_cvref_t<decltype(graph_part.part)>;

  KASPAN_STATISTIC_ADD("n", graph_part.part.n);
  KASPAN_STATISTIC_ADD("local_n", graph_part.part.local_n());
  KASPAN_STATISTIC_ADD("m", graph_part.m);
  KASPAN_STATISTIC_ADD("local_fw_m", graph_part.local_fw_m);
  KASPAN_STATISTIC_ADD("local_bw_m", graph_part.local_bw_m);
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  // Create HPCGraph data structure from kaspan graph
  hpc_graph_data hpc_data;
  part_t         part_copy;
  index_t        local_fw_m_copy = 0;
  index_t        local_bw_m_copy = 0;
  {
    KASPAN_STATISTIC_PUSH("adapter");
    hpc_data = create_hpc_graph_from_graph_part(
      graph_part.part, graph_part.m, graph_part.local_fw_m, graph_part.local_bw_m, graph_part.fw_head, graph_part.fw_csr, graph_part.bw_head, graph_part.bw_csr);
    KASPAN_STATISTIC_POP();

    // Save partitioning info (lightweight) before releasing kaspan memory
    part_copy       = graph_part.part;
    local_fw_m_copy = graph_part.local_fw_m;
    local_bw_m_copy = graph_part.local_bw_m;
  }

  KASPAN_STATISTIC_ADD("memory_after_adapter", get_resident_set_bytes());

  // Release kaspan memory before SCC benchmark to ensure fair memory measurement
  // This prevents double allocation and allows accurate memory footprint measurement
  graph_part = std::remove_cvref_t<decltype(graph_part)>{}; // Explicitly release the kaspan graph memory

  KASPAN_STATISTIC_ADD("memory_after_release", get_resident_set_bytes());

  mpi_basic::barrier();

  KASPAN_CALLGRIND_START_INSTRUMENTATION();
  KASPAN_STATISTIC_PUSH("scc");

  // Initialize ghost cells as part of SCC benchmark
  // Ghost cells are an accelerator structure specific to HPCGraph
  KASPAN_STATISTIC_PUSH("ghost_init");
  initialize_ghost_cells(hpc_data, part_copy, local_fw_m_copy, local_bw_m_copy);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_PUSH("pivot");
  degree max_degree{ .degree_product = std::numeric_limits<index_t>::min(), .u = std::numeric_limits<vertex_t>::min() };
  // Pivot selection using HPCGraph data (kaspan memory has been released)
  for (uint64_t k = 0; k < hpc_data.g.n_local; ++k) {
    auto const out_degree     = hpc_data.g.out_degree_list[k + 1] - hpc_data.g.out_degree_list[k];
    auto const in_degree      = hpc_data.g.in_degree_list[k + 1] - hpc_data.g.in_degree_list[k];
    auto const degree_product = out_degree * in_degree;
    if (kaspan::integral_cast<index_t>(degree_product) > max_degree.degree_product) {
      max_degree.degree_product = kaspan::integral_cast<index_t>(degree_product);
      max_degree.u              = kaspan::integral_cast<vertex_t>(hpc_data.g.local_unmap[k]);
    }
  }
  auto const pivot = pivot_selection(max_degree);
  KASPAN_STATISTIC_POP();
  char output_file[] = "hpc_scc_output.txt";
  scc_dist(&hpc_data.g, &hpc_data.comm, &hpc_data.q, kaspan::integral_cast<uint64_t>(pivot), output_file);
  KASPAN_STATISTIC_POP();
  KASPAN_CALLGRIND_STOP_INSTRUMENTATION();

  KASPAN_STATISTIC_ADD("memory_after_scc", get_resident_set_bytes());
  destroy_hpc_graph_data(&hpc_data);
}

int
main(int argc, char** argv)
{
  auto const* const kagen_option_string = arg_select_optional_str(argc, argv, "--kagen_option_string");
  auto const* const manifest_file       = arg_select_optional_str(argc, argv, "--manifest_file");
  auto const* const output_file         = arg_select_str(argc, argv, "--output_file", usage);
  auto const        threads             = arg_select_default_int(argc, argv, "--threads", 1);

  if ((kagen_option_string == nullptr ^ manifest_file == nullptr) == 0) {
    usage(argc, argv);
    std::exit(1);
  }

  KASPAN_DEFAULT_INIT();
  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));

  // Set OpenMP to single thread only
  omp_set_num_threads(threads);

  // Initialize HPCGraph global variables
  MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("world_rank", mpi_basic::world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_basic::world_size);
  KASPAN_STATISTIC_ADD("memcheck", KASPAN_MEMCHECK);
  KASPAN_STATISTIC_ADD("callgrind", KASPAN_CALLGRIND);

  if (kagen_option_string != nullptr) {
    KASPAN_STATISTIC_PUSH("kagen");
    auto kagen_graph = kagen_graph_part(kagen_option_string);
    KASPAN_STATISTIC_POP();
    benchmark(std::move(kagen_graph));
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest = manifest::load(manifest_file);
    ASSERT_LT(manifest.graph_node_count, std::numeric_limits<vertex_t>::max());
    auto const part           = balanced_slice_part{ kaspan::integral_cast<vertex_t>(manifest.graph_node_count) };
    auto       manifest_graph = load_graph_part_from_manifest(part, manifest);
    KASPAN_STATISTIC_POP();
    benchmark(std::move(manifest_graph));
  }
}
