#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/debug/assert.hpp>
#include <vector>

using namespace kaspan;

int main() {
    // Test empty bidi_graph
    {
        bidi_graph g(0, 0);
        ASSERT_EQ(g.n, 0);
        ASSERT_EQ(g.m, 0);
        ASSERT_EQ(g.fw.head, nullptr);
        ASSERT_EQ(g.bw.head, nullptr);
        g.debug_check();
        g.debug_validate();
    }

    // Test a simple bidi graph: 0 <-> 1
    {
        vertex_t n = 2;
        vertex_t m = 1;
        bidi_graph g(n, m);
        
        // Forward: 0 -> 1
        g.fw.head[0] = 0;
        g.fw.head[1] = 1;
        g.fw.head[2] = 1;
        g.fw.csr[0] = 1;
        
        // Backward: 1 -> 0
        g.bw.head[0] = 0;
        g.bw.head[1] = 0;
        g.bw.head[2] = 1;
        g.bw.csr[0] = 0;
        
        g.debug_check();
        g.debug_validate();
        
        ASSERT_EQ(g.outdegree(0), 1);
        ASSERT_EQ(g.indegree(1), 1);
        ASSERT_EQ(g.outdegree(1), 0);
        ASSERT_EQ(g.indegree(0), 0);
        
        bool called_fw = false;
        g.each_v(0, [&](vertex_t v) {
            ASSERT_EQ(v, 1);
            called_fw = true;
        });
        ASSERT(called_fw);
        
        bool called_bw = false;
        g.each_bw_v(1, [&](vertex_t v) {
            ASSERT_EQ(v, 0);
            called_bw = true;
        });
        ASSERT(called_bw);
    }

    return 0;
}
