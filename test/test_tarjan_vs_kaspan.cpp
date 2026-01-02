#include "memory/buffer.hpp"
#include "memory/borrow.hpp"
#include "scc/tarjan.hpp"
#include "debug/assert_in_range.hpp"
#include "debug/assert_eq.hpp"
#include <briefkasten/noop_indirection.hpp>
#include <debug/sub_process.hpp>
#include <scc/adapter/kagen.hpp>
#include <scc/allgather_graph.hpp>
#include <scc/async/scc.hpp>
#include <scc/base.hpp>
#include <scc/scc.hpp>

#include <mpi.h>


int
main(int argc, char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  auto const graph_part = kagen_graph_part("rmat;directed;N=16;M=18;a=0.25;b=0.25;c=0.25");
  auto const graph      = allgather_graph(graph_part.part, graph_part.m, graph_part.local_fw_m, graph_part.fw_head, graph_part.fw_csr);

  // Test sync version
  {
    auto const scc_id_buffer = make_buffer<vertex_t>(graph_part.part.local_n());
    auto*      scc_id_access = scc_id_buffer.data();
    auto*      kaspan_scc_id = borrow_array_clean<vertex_t>(&scc_id_access, graph_part.part.local_n());
    scc(graph_part.part, graph_part.fw_head, graph_part.fw_csr, graph_part.bw_head, graph_part.bw_csr, kaspan_scc_id);

    tarjan(graph.n, graph.fw_head, graph.fw_csr, [&](auto* beg, auto* end) {
      vertex_t min_u = graph.n;
      for (auto it = beg; it < end; ++it) {
        ASSERT_IN_RANGE(*it, 0, graph.n);
        min_u = std::min(min_u, *it);
      }
      for (auto it = beg; it < end; ++it) {
        if (graph_part.part.has_local(*it)) {
          auto const k = graph_part.part.to_local(*it);
          ASSERT_EQ(kaspan_scc_id[k], min_u);
        }
      }
    });
  }

  // Test async version with NoopIndirectionScheme
  {
    auto const scc_id_buffer = make_buffer<vertex_t>(graph_part.part.local_n());
    auto*      scc_id_access = scc_id_buffer.data();
    auto*      async_scc_id  = borrow_array_clean<vertex_t>(&scc_id_access, graph_part.part.local_n());
    async::scc<briefkasten::NoopIndirectionScheme>(graph_part.part, graph_part.fw_head, graph_part.fw_csr, graph_part.bw_head, graph_part.bw_csr, async_scc_id);

    tarjan(graph.n, graph.fw_head, graph.fw_csr, [&](auto* beg, auto* end) {
      vertex_t min_u = graph.n;
      for (auto it = beg; it < end; ++it) {
        ASSERT_IN_RANGE(*it, 0, graph.n);
        min_u = std::min(min_u, *it);
      }
      for (auto it = beg; it < end; ++it) {
        if (graph_part.part.has_local(*it)) {
          auto const k = graph_part.part.to_local(*it);
          ASSERT_EQ(async_scc_id[k], min_u);
        }
      }
    });
  }

  // // Test async version with GridIndirectionScheme
  // {
  //   auto const scc_id_buffer = make_buffer<vertex_t>(graph_part.part.local_n());
  //   auto* scc_id_access = scc_id_buffer.data();
  //   auto* async_indirect_scc_id = borrow_array_clean<vertex_t>(&scc_id_access, graph_part.part.local_n());
  //   async::scc<briefkasten::GridIndirectionScheme>(
  //     graph_part.part,
  //     graph_part.fw_head,
  //     graph_part.fw_csr,
  //     graph_part.bw_head,
  //     graph_part.bw_csr,
  //     async_indirect_scc_id);
  //
  //   tarjan(graph.n, graph.fw_head, graph.fw_csr, [&](auto* beg, auto* end) {
  //     vertex_t min_u = graph.n;
  //     for (auto it = beg; it < end; ++it) {
  //       ASSERT_IN_RANGE(*it, 0, graph.n);
  //       min_u = std::min(min_u, *it);
  //     }
  //     for (auto it = beg; it < end; ++it) {
  //       if (graph_part.part.has_local(*it)) {
  //         auto const k = graph_part.part.to_local(*it);
  //         ASSERT_EQ(async_indirect_scc_id[k], min_u);
  //       }
  //     }
  //   });
  // }
}
