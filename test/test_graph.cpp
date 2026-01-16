#include <kaspan/graph/graph.hpp>
#include <kaspan/debug/assert.hpp>
#include <vector>

using namespace kaspan;

int main() {
    // Test empty graph
    {
        graph g(0, 0);
        ASSERT_EQ(g.n, 0);
        ASSERT_EQ(g.m, 0);
        ASSERT_EQ(g.head, nullptr);
        ASSERT_EQ(g.csr, nullptr);
        g.debug_check();
        g.debug_validate();
    }

    // Test a simple graph: 0 -> 1, 0 -> 2, 1 -> 2
    {
        vertex_t n = 3;
        vertex_t m = 3;
        graph g(n, m);
        
        // Setup CSR
        g.head[0] = 0;
        g.head[1] = 2; // 0 has 2 neighbors
        g.head[2] = 3; // 1 has 1 neighbor
        g.head[3] = 3; // 2 has 0 neighbors
        
        g.csr[0] = 1;
        g.csr[1] = 2;
        g.csr[2] = 2;
        
        g.debug_check();
        g.debug_validate();
        
        ASSERT_EQ(g.outdegree(0), 2);
        ASSERT_EQ(g.outdegree(1), 1);
        ASSERT_EQ(g.outdegree(2), 0);
        
        auto range0 = g.csr_range(0);
        ASSERT_EQ(range0.size(), 2);
        ASSERT_EQ(range0[0], 1);
        ASSERT_EQ(range0[1], 2);
        
        std::vector<std::pair<vertex_t, vertex_t>> edges;
        g.each_uv([&](vertex_t u, vertex_t v) {
            edges.push_back({u, v});
        });
        
        ASSERT_EQ(edges.size(), 3);
        ASSERT_EQ(edges[0].first, 0); ASSERT_EQ(edges[0].second, 1);
        ASSERT_EQ(edges[1].first, 0); ASSERT_EQ(edges[1].second, 2);
        ASSERT_EQ(edges[2].first, 1); ASSERT_EQ(edges[2].second, 2);
    }

    return 0;
}
