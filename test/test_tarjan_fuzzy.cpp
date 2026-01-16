#include <kaspan/debug/assert.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/fuzzy.hpp>
#include <kaspan/scc/tarjan.hpp>

#include <cstdio>
#include <span>

using namespace kaspan;

int
main()
{
  auto rng  = std::mt19937{ std::random_device{}() };
  auto dist = std::uniform_int_distribution<u64>{};

  for (vertex_t n = 0; n < 300; ++n) {
    for (i32 i = 0; i < 10; ++i) {
      for (auto const d : { 0.5, 1.0, 4.0 }) {

        auto const [scc_id, bg] = fuzzy_global_scc_id_and_graph(dist(rng), n, d);

        vertex_t scc_comp_count = 0;
        for (vertex_t u = 0; u < n; ++u) {
          scc_comp_count += scc_id[u] == u;
        }

        vertex_t tarjan_comp_count = 0;
        tarjan(bg.fw_view(), [&](auto const* beg, auto const* end) -> void {
          auto const id = *std::min_element(beg, end);
          for (auto const u : std::span{ beg, end }) {
            ASSERT_EQ(scc_id[u], id);
          }
          ++tarjan_comp_count;
        });

        ASSERT_EQ(scc_comp_count, tarjan_comp_count);
      }
    }
  }
}
