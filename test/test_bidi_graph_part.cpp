#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/debug/assert.hpp>
#include <vector>

using namespace kaspan;

int main() {
    // Test empty bidi_graph_part
    {
        single_world_part part(0);
        bidi_graph_part<single_world_part> bgp(part, 0, 0);
        ASSERT_EQ(bgp.part.n, 0);
        ASSERT_EQ(bgp.local_fw_m, 0);
        ASSERT_EQ(bgp.local_bw_m, 0);
        bgp.debug_check();
        bgp.debug_validate();
    }

    // Test a simple partitioned bidi graph
    {
        vertex_t n = 2;
        single_world_part part(n);
        bidi_graph_part<single_world_part> bgp(part, 1, 1);
        
        // Forward: 0 -> 1
        bgp.fw.head[0] = 0;
        bgp.fw.head[1] = 1;
        bgp.fw.head[2] = 1;
        bgp.fw.csr[0] = 1;
        
        // Backward: 1 -> 0
        bgp.bw.head[0] = 0;
        bgp.bw.head[1] = 0;
        bgp.bw.head[2] = 1;
        bgp.bw.csr[0] = 0;
        
        bgp.debug_check();
        bgp.debug_validate();
        
        ASSERT_EQ(bgp.outdegree(0), 1);
        ASSERT_EQ(bgp.indegree(1), 1);
        
        std::vector<vertex_t> neighbors;
        bgp.each_v(0, [&](vertex_t v) {
            neighbors.push_back(v);
        });
        ASSERT_EQ(neighbors.size(), 1);
        ASSERT_EQ(neighbors[0], 1);
        
        neighbors.clear();
        bgp.each_bw_v(1, [&](vertex_t v) {
            neighbors.push_back(v);
        });
        ASSERT_EQ(neighbors.size(), 1);
        ASSERT_EQ(neighbors[0], 0);
    }

    return 0;
}
