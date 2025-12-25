#include "kaspan_adapter.hpp"

#include <debug/process.hpp>
#include <debug/statistic.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/adapter/manifest.hpp>
#include <scc/pivot_selection.hpp>
#include <util/arg_parse.hpp>

#include "scc.h"

#include <mpi.h>
#include <omp.h>

#include <cstdio>
#include <print>

// Global variables required by HPCGraph
int  procid, nprocs;
bool verbose = false;
bool debug   = false;
bool verify  = false;
bool output  = false;

void
usage(int /* argc */, char** argv)
{
  std::println(
    "usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file "
    "<output_file>",
    argv[0]);
}

template<WorldPartConcept Part>
void
benchmark_impl(GraphPart<Part> const& graph_part)
{
  KASPAN_STATISTIC_ADD("n", graph_part.part.n);
  KASPAN_STATISTIC_ADD("local_n", graph_part.part.local_n());
  KASPAN_STATISTIC_ADD("m", graph_part.m);
  KASPAN_STATISTIC_ADD("local_fw_m", graph_part.local_fw_m);
  KASPAN_STATISTIC_ADD("local_bw_m", graph_part.local_bw_m);

  KASPAN_STATISTIC_PUSH("adapter");
  auto hpc_data = create_hpc_graph_from_graph_part(graph_part);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_ADD("memory_after_adapter", get_resident_set_bytes());

  MPI_Barrier(MPI_COMM_WORLD);

  KASPAN_STATISTIC_PUSH("scc");
  KASPAN_STATISTIC_PUSH("pivot");
  Degree max_degree{
    .degree_product = std::numeric_limits<index_t>::min(),
    .u              = std::numeric_limits<vertex_t>::min()
  };
  for (vertex_t k = 0; k < graph_part.part.local_n(); ++k) {
    auto const out_degree     = graph_part.fw_head[k + 1] - graph_part.fw_head[k];
    auto const in_degree      = graph_part.bw_head[k + 1] - graph_part.bw_head[k];
    auto const degree_product = out_degree * in_degree;
    if (degree_product > max_degree.degree_product) {
      max_degree.degree_product = degree_product;
      max_degree.u              = graph_part.part.to_global(k);
    }
  }
  auto const pivot = pivot_selection(max_degree);
  KASPAN_STATISTIC_POP();
  char output_file[] = "hpc_scc_output.txt";
  scc_dist(&hpc_data.g, &hpc_data.comm, &hpc_data.q, static_cast<uint64_t>(pivot), output_file);
  KASPAN_STATISTIC_POP();

  KASPAN_STATISTIC_ADD("memory_after_scc", get_resident_set_bytes());

  destroy_hpc_graph_data(&hpc_data);
}

template<typename T>
void
benchmark(T const& graph_part)
{
  using Part = std::remove_cvref_t<decltype(graph_part.part)>;
  // Create a non-owning GraphPart view
  GraphPart<Part> view;
  view.part       = graph_part.part;
  view.m          = graph_part.m;
  view.local_fw_m = graph_part.local_fw_m;
  view.local_bw_m = graph_part.local_bw_m;
  view.fw_head    = graph_part.fw_head;
  view.bw_head    = graph_part.bw_head;
  view.fw_csr     = graph_part.fw_csr;
  view.bw_csr     = graph_part.bw_csr;
  benchmark_impl(view);
}

int
main(int argc, char** argv)
{
  auto const kagen_option_string = arg_select_optional_str(argc, argv, "--kagen_option_string");
  auto const manifest_file       = arg_select_optional_str(argc, argv, "--manifest_file");
  auto const output_file = arg_select_str(argc, argv, "--output_file", usage);

  if (not(kagen_option_string == nullptr ^ manifest_file == nullptr)) {
    usage(argc, argv);
    std::exit(1);
  }

  KASPAN_DEFAULT_INIT();
  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file));

  // Set OpenMP to single thread only
  omp_set_num_threads(1);

  // Initialize HPCGraph global variables
  MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());
  KASPAN_STATISTIC_ADD("world_rank", mpi_world_rank);
  KASPAN_STATISTIC_ADD("world_size", mpi_world_size);

  if (kagen_option_string != nullptr) {
    KASPAN_STATISTIC_PUSH("kagen");
    auto const kagen_graph = kagen_graph_part(kagen_option_string);
    KASPAN_STATISTIC_POP();
    benchmark(kagen_graph);
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest = Manifest::load(manifest_file);
    ASSERT_LT(manifest.graph_node_count, std::numeric_limits<vertex_t>::max());
    auto const part           = BalancedSlicePart{ static_cast<vertex_t>(manifest.graph_node_count) };
    auto const manifest_graph = load_graph_part_from_manifest(part, manifest);
    KASPAN_STATISTIC_POP();
    benchmark(manifest_graph);
  }
}
