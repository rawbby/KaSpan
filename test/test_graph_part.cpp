#include "kaspan/graph/single_part.hpp"

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <vector>

using namespace kaspan;

int
main()
{
  // Test empty graph_part
  {
    single_part part(0);
    graph_part  gp(part, 0);
    ASSERT_EQ(gp.part.n(), 0);
    ASSERT_EQ(gp.local_m, 0);
    ASSERT_EQ(gp.head, nullptr);
    ASSERT_EQ(gp.csr, nullptr);
    gp.debug_check();
    gp.debug_validate();
  }

  // Test a simple partitioned graph (single partition for simplicity)
  // Global graph: 0 -> 1, 0 -> 2, 1 -> 2
  {
    vertex_t    n = 3;
    vertex_t    m = 3;
    single_part part(n);
    graph_part  gp(part, m);

    // Setup CSR
    gp.head[0] = 0;
    gp.head[1] = 2; // k=0 (u=0) has 2 neighbors
    gp.head[2] = 3; // k=1 (u=1) has 1 neighbor
    gp.head[3] = 3; // k=2 (u=2) has 0 neighbors

    gp.csr[0] = 1;
    gp.csr[1] = 2;
    gp.csr[2] = 2;

    gp.debug_check();
    gp.debug_validate();

    ASSERT_EQ(gp.outdegree(0), 2);

    std::vector<vertex_t> neighbors;
    gp.each_v(0, [&](vertex_t v) {
      neighbors.push_back(v);
    });
    ASSERT_EQ(neighbors.size(), 2);
    ASSERT_EQ(neighbors[0], 1);
    ASSERT_EQ(neighbors[1], 2);

    bool called = false;
    gp.each_kuv([&](vertex_t k, vertex_t u, vertex_t v) {
      if (k == 1) {
        ASSERT_EQ(u, 1);
        ASSERT_EQ(v, 2);
        called = true;
      }
    });
    ASSERT(called);
  }

  return 0;
}
