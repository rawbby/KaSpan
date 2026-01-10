#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/mpi_basic/allreduce_max_time.hpp>
#include <kaspan/mpi_basic/allreduce_single.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/scc/async/scc.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/fuzzy.hpp>
#include <kaspan/scc/part.hpp>
#include <kaspan/scc/scc.hpp>

#include <briefkasten/noop_indirection.hpp>

#include <cmath>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <utility>

using namespace kaspan;

template<typename Graph>
void
verify_scc_id(Graph const& graph, vertex_t const* scc_id_orig, vertex_t* scc_id)
{
  MPI_Barrier(MPI_COMM_WORLD);

  auto const& part    = graph.part;
  auto const  local_n = part.local_n();

  for (vertex_t k = 0; k < part.local_n(); ++k) {
    if (scc_id_orig[k] != scc_id[k]) {
      // clang-format off
      auto const [beg, end] = [&] -> std::pair<vertex_t, vertex_t> {
        if (local_n <= 50) {    return { 0, local_n };
}
        if (k < 25) {           return { 0, 50 };
}
        if (k > local_n - 25) { return { local_n - 50, local_n };
        } else {                  return { k - 25, k + 25 };
}
      }();
      // clang-format on

      auto const w = static_cast<vertex_t>(std::log10(std::max(2, part.to_global(end)) - 1) + 1);
      auto const p = std::string{ " " } + std::string(w, ' ');
      auto const m = std::string{ " " } + std::string(w, '^');

      std::stringstream idx_stream;
      std::stringstream ref_stream;
      std::stringstream rlt_stream;
      std::stringstream mrk_stream;

      idx_stream << "  index       :";
      ref_stream << "  scc_id_orig :";
      rlt_stream << "  scc_id      :";
      mrk_stream << "               ";

      for (vertex_t it = beg; it < end; ++it) {
        idx_stream << ' ' << std::right << std::setw(w) << part.to_global(it);
        ref_stream << ' ' << std::right << std::setw(w) << scc_id_orig[it];
        rlt_stream << ' ' << std::right << std::setw(w) << scc_id[it];
        mrk_stream << (scc_id_orig[it] == scc_id[it] ? p : m);
      }

      ASSERT_EQ(graph.scc_id_part[k],
                scc_id[k],
                "k={}; u={}; n={}; rank={}; size={};\n{}\n{}\n{}\n{}",
                k,
                part.to_global(k),
                part.n,
                mpi_basic::world_rank,
                mpi_basic::world_size,
                idx_stream.str(),
                ref_stream.str(),
                rlt_stream.str(),
                mrk_stream.str());
    }
  }
}

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  for (int run = 1; run <= 40; ++run) {
    for (vertex_t n : { 1, 6, 8, 16, 33, 64 }) {
      DEBUG_ASSERT_EQ(n, mpi_basic::allreduce_single(n, MPI_MAX));

      auto const seed  = mpi_basic::allreduce_max_time();
      auto const part  = balanced_slice_part{ n };
      auto const graph = fuzzy_local_scc_id_and_graph(seed, part);

      // Test sync version
      {
        auto  buffer = make_buffer<vertex_t>(part.local_n());
        auto* memory = buffer.data();
        auto* scc_id = borrow_array<vertex_t>(&memory, part.local_n());
        scc(part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);
        verify_scc_id(graph, graph.scc_id_part, scc_id);
      }

      // Test async version with NoopIndirectionScheme
      {
        auto  buffer = make_buffer<vertex_t>(part.local_n());
        auto* memory = buffer.data();
        auto* scc_id = borrow_array<vertex_t>(&memory, part.local_n());
        async::scc<briefkasten::NoopIndirectionScheme>(part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);
        verify_scc_id(graph, graph.scc_id_part, scc_id);
      }
    }
  }
}
