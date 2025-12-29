#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <scc/async/scc.hpp>
#include <scc/fuzzy.hpp>
#include <scc/scc.hpp>
#include <util/scope_guard.hpp>

#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/noop_indirection.hpp>

#include <iomanip>

int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  for (vertex_t n = 16; n < 128; n += 8) {

    DEBUG_ASSERT_EQ(n, mpi_basic_allreduce_single(n, MPI_MAX));

    auto const seed = mpi_basic_allreduce_max_time();
    auto const part = BalancedSlicePart{ n };

    auto const graph = fuzzy_local_scc_id_and_graph(seed, part);

 // // Test sync version
 // {
 //   auto  buffer = make_buffer<vertex_t>(part.local_n());
 //   auto* memory = buffer.data();
 //   auto* scc_id = borrow_array<vertex_t>(&memory, part.local_n());

 //   scc(part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);

 //   std::stringstream ss;
 //   ss << "  index         :";
 //   for (vertex_t k = 0; k < part.local_n(); ++k)
 //     ss << ' ' << std::right << std::setw(2) << graph.part.to_global(k);
 //   ss << "\n  scc_id_orig   :";
 //   for (vertex_t k = 0; k < part.local_n(); ++k)
 //     ss << ' ' << std::right << std::setw(2) << graph.scc_id_part[k];
 //   ss << "\n  scc_id_kaspan :";
 //   for (vertex_t k = 0; k < part.local_n(); ++k)
 //     ss << ' ' << std::right << std::setw(2) << scc_id[k];
 //   ss << "\n                 ";
 //   for (vertex_t k = 0; k < part.local_n(); ++k)
 //     ss << (graph.scc_id_part[k] == scc_id[k] ? "   " : " ^^");
 //   ss << std::endl;

 //   auto const status_str = ss.str();

 //   MPI_Barrier(MPI_COMM_WORLD);

 //   for (size_t k = 0; k < part.local_n(); ++k) {
 //     ASSERT_EQ(graph.scc_id_part[k], scc_id[k], "k={} / i={}\n{}", k, part.to_global(k), status_str.c_str());
 //   }
 // }

    // Test async version with NoopIndirectionScheme
    {
      auto  buffer = make_buffer<vertex_t>(part.local_n());
      auto* memory = buffer.data();
      auto* scc_id = borrow_array<vertex_t>(&memory, part.local_n());

      async::scc<briefkasten::NoopIndirectionScheme>(part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);

      std::stringstream ss;
      ss << "  index             :";
      for (vertex_t k = 0; k < part.local_n(); ++k)
        ss << ' ' << std::right << std::setw(2) << graph.part.to_global(k);
      ss << "\n  scc_id_orig       :";
      for (vertex_t k = 0; k < part.local_n(); ++k)
        ss << ' ' << std::right << std::setw(2) << graph.scc_id_part[k];
      ss << "\n  scc_id_async_noop :";
      for (vertex_t k = 0; k < part.local_n(); ++k)
        ss << ' ' << std::right << std::setw(2) << scc_id[k];
      ss << "\n                     ";
      for (vertex_t k = 0; k < part.local_n(); ++k)
        ss << (graph.scc_id_part[k] == scc_id[k] ? "   " : " ^^");
      ss << std::endl;

      auto const status_str = ss.str();

      MPI_Barrier(MPI_COMM_WORLD);

      for (size_t k = 0; k < part.local_n(); ++k) {
        ASSERT_EQ(graph.scc_id_part[k], scc_id[k], "async_noop: k={} / i={}\n{}", k, part.to_global(k), status_str.c_str());
      }
    }

//     // Test async version with GridIndirectionScheme
//     {
//       auto  buffer = make_buffer<vertex_t>(part.local_n());
//       auto* memory = buffer.data();
//       auto* scc_id = borrow_array<vertex_t>(&memory, part.local_n());
//
//       async::scc<briefkasten::GridIndirectionScheme>(part, graph.fw_head, graph.fw_csr, graph.bw_head, graph.bw_csr, scc_id);
//
//       std::stringstream ss;
//       ss << "  index             :";
//       for (vertex_t k = 0; k < part.local_n(); ++k)
//         ss << ' ' << std::right << std::setw(2) << graph.part.to_global(k);
//       ss << "\n  scc_id_orig       :";
//       for (vertex_t k = 0; k < part.local_n(); ++k)
//         ss << ' ' << std::right << std::setw(2) << graph.scc_id_part[k];
//       ss << "\n  scc_id_async_grid :";
//       for (vertex_t k = 0; k < part.local_n(); ++k)
//         ss << ' ' << std::right << std::setw(2) << scc_id[k];
//       ss << "\n                     ";
//       for (vertex_t k = 0; k < part.local_n(); ++k)
//         ss << (graph.scc_id_part[k] == scc_id[k] ? "   " : " ^^");
//       ss << std::endl;
//
//       auto const status_str = ss.str();
//
//       MPI_Barrier(MPI_COMM_WORLD);
//
//       for (size_t k = 0; k < part.local_n(); ++k) {
//         ASSERT_EQ(graph.scc_id_part[k], scc_id[k], "async_grid: k={} / i={}\n{}", k, part.to_global(k), status_str.c_str());
//       }
//     }
  }
}
