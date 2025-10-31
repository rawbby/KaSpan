#include <debug/Process.hpp>
#include <debug/Statistic.hpp>
#include <graph/Graph.hpp>
#include <graph/KaGen.hpp>

#include <kamping/collectives/barrier.hpp>
#include <kamping/communicator.hpp>

#include <mpi.h>
#include <omp.h>

[[noreturn]] void
usage()
{
  std::cout << "usage: ./<exe> --num_threads <num_threads> --kagen_option_string <kagen_option_string> --output_file <output_file>"
            << std::endl;
  std::exit(1);
}

std::string
select_kagen_option_string(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--kagen_option_string") == 0)
      return argv[i + 1];
  usage();
}

int
select_num_threads(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--num_threads") == 0)
      return std::stoi(argv[i + 1]);
  return omp_get_num_threads();
}

std::string
select_output_file(int argc, char** argv)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], "--output_file") == 0)
      return argv[i + 1];
  usage();
}

#include <algorithm>
#include <omp.h>
#include <vector>

namespace parallel {

template<WorldPartConcept Part>
VoidResult
scc(
  Part const& part,

  index_t const* __restrict fw_head,
  vertex_t const* __restrict fw_csr,
  index_t const* __restrict bw_head,
  vertex_t const* __restrict bw_csr,

  vertex_t* __restrict scc_id,
  void* memory)
{
  vertex_t const local_n = static_cast<vertex_t>(part.local_n());
  RESULT_TRY(auto buffer1, VertexBuffer::create(local_n));
  RESULT_TRY(auto buffer2, VertexBuffer::create(local_n));

  {
    auto wcc_work = buffer1.range();
    auto wcc_best = buffer2.range();

#pragma omp parallel for schedule(static)
    for (vertex_t u = 0; u < local_n; ++u)
      wcc_work[u] = u;

    while (true) {
      bool changed = false;

#pragma omp parallel for schedule(static) reduction(|| : changed)
      for (vertex_t k = 0; k < local_n; ++k) {
        auto best = wcc_work[k];

        {
          auto const beg = fw_head[k];
          auto const end = fw_head[k + 1];
          for (index_t it = beg; it < end; ++it)
            if (auto const v = fw_csr[it]; part.has_local(v))
              best = std::min(best, wcc_work[part.to_local(v)]);
        }

        {
          auto const beg = bw_head[k];
          auto const end = bw_head[k + 1];
          for (index_t it = beg; it < end; ++it)
            if (auto const v = bw_csr[it]; part.has_local(v))
              best = std::min(best, wcc_work[part.to_local(v)]);
        }

        wcc_best[k] = best;
        if (best != wcc_work[k])
          changed = true;
      }

      if (not changed)
        break;

      std::swap(wcc_work, wcc_best);
    }
  }

  return VoidResult::success();
}

} // namespace parallel

int
main(int argc, char** argv)
{
  auto const kagen_option_string = select_kagen_option_string(argc, argv);
  auto const output_file         = select_output_file(argc, argv);
  auto const num_threads         = select_num_threads(argc, argv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  SCOPE_GUARD(KASPAN_STATISTIC_MPI_WRITE_JSON(output_file.c_str()));
  KASPAN_STATISTIC_SCOPE("benchmark");
  KASPAN_STATISTIC_ADD("memory", get_resident_set_bytes());

  auto comm = kamping::Communicator{};
  comm.barrier();

  KASPAN_STATISTIC_ADD("world_rank", comm.rank());
  KASPAN_STATISTIC_ADD("world_size", comm.size());

  KASPAN_STATISTIC_PUSH("kagen");
  ASSERT_TRY(auto const graph_part, KaGenGraphPart(comm, kagen_option_string.c_str()));
  KASPAN_STATISTIC_POP();

  ASSERT_TRY(auto scc_id, U64Buffer::create(graph_part.part.n));
  ASSERT_TRY(parallel::scc(graph_part.part,
                           static_cast<index_t const*>(graph_part.fw_head.data()),
                           static_cast<vertex_t const*>(graph_part.fw_csr.data()),
                           static_cast<index_t const*>(graph_part.bw_head.data()),
                           static_cast<vertex_t const*>(graph_part.bw_csr.data()),
                           static_cast<vertex_t*>(scc_id.data())));

#if KASPAN_STATISTIC
  size_t local_component_count = 0;
  for (vertex_t k = 0; k < graph_part.part.local_n(); ++k)
    if (scc_id.get(k) == graph_part.part.to_global(k))
      ++local_component_count;

  auto const global_component_count = comm.allreduce_single(kamping::send_buf(local_component_count), kamping::op(MPI_SUM));
  KASPAN_STATISTIC_ADD("local_component_count", local_component_count);
  KASPAN_STATISTIC_ADD("global_component_count", global_component_count);
#endif
}
