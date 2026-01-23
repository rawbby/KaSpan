#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/scc/adapter/kagen.hpp>
#include <kaspan/scc/adapter/manifest.hpp>
#include <kaspan/scc/pivot.hpp>
#include <kaspan/util/arg_parse.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <adapter/base.hpp>
#include <adapter/kagen.hpp>
#include <adapter/manifest.hpp>

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
usage(
  int /* argc */,
  char** argv)
{
  std::println("usage: {} (--kagen_option_string <kagen_option_string> | --manifest_file <manifest_file>) --output_file "
               "<output_file> [--threads <threads>]",
               argv[0]);
}

int
main(
  int    argc,
  char** argv)
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

  hpc_graph_data hpc_data;

  if (kagen_option_string != nullptr) {
    KASPAN_STATISTIC_PUSH("kagen");
    hpc_data = kagen_hpc_graph_part(kagen_option_string);
    KASPAN_STATISTIC_POP();
  } else {
    KASPAN_STATISTIC_PUSH("load");
    auto const manifest = manifest::load(manifest_file);
    auto const part     = balanced_slice_part{ kaspan::integral_cast<vertex_t>(manifest.graph_node_count) };
    hpc_data            = load_hpc_graph_from_manifest(part, manifest);
    KASPAN_STATISTIC_POP();
  }

  // benchmark
  {
    KASPAN_STATISTIC_ADD("n", hpc_data.g.n);
    KASPAN_STATISTIC_ADD("local_n", hpc_data.g.n_local);
    KASPAN_STATISTIC_ADD("m", hpc_data.g.m);
    KASPAN_STATISTIC_ADD("local_fw_m", hpc_data.g.m_local_out);
    KASPAN_STATISTIC_ADD("local_bw_m", hpc_data.g.m_local_in);
    KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

    mpi_basic::barrier();

    KASPAN_CALLGRIND_START_INSTRUMENTATION();
    KASPAN_STATISTIC_PUSH("scc");

    KASPAN_STATISTIC_PUSH("init");
    // Initialize ghost cells, maps, etc as part of SCC benchmark
    // these are accelerator structurea specific to HPCGraph
    hpc_data.initialize();
    KASPAN_STATISTIC_POP();

    auto scc_id = make_array<u64>(hpc_data.g.n_total);
    scc_dist(&hpc_data.g, &hpc_data.comm, &hpc_data.q, hpc_data.g.max_degree_vert, scc_id.data());
    KASPAN_STATISTIC_POP();

    KASPAN_STATISTIC_ADD("memory_after_scc", get_resident_set_bytes());

    KASPAN_CALLGRIND_STOP_INSTRUMENTATION();
  }
}
