#include <scc/fuzzy.hpp>
#include <iostream>
#include <iomanip>

int main() {
    for (u64 n = 2; n <= 10; ++n) {
        double max_d = static_cast<double>(n * n - n) / n;
        std::cout << "n=" << n << ", max reachable degree (theoretical)=" << max_d << std::endl;
        
        // Try to reach a degree slightly above max_d
        // Note: The fuzzy generator might hang or fail if it can't reach the degree
        // We just want to confirm the math.
    }
    return 0;
}
