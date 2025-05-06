#include "./include/csr/csr.h"

#include <iostream>

int
main()
{
  {
    auto builder = csr::builder{};
    builder.push(1, 2);
    builder.push(2, 3);
    builder.push(3, 4);
    builder.push(4, 3);
    builder.push(5, 2);
    builder.push(6, 3);

    auto f = std::fopen("test.csr", "wb+");
    if (!f)
      std::exit(1);

    builder.serialize(f);
    std::fclose(f);
  }

  constexpr int node_count = 5;
  for (int node_index = 0; node_index < node_count; ++node_index) {

    const auto f = std::fopen("test.csr", "rb");
    csr::load_distributed(f, node_index, node_count, [f](auto csr) {
      std::fclose(f); // already loaded

      for (const std::uint64_t from_node : csr.local_nodes())
        for (const std::uint64_t to_node : csr[from_node])
          std::cout << from_node << ',' << to_node << '\n';
    });
    std::cout << "---\n";
  }
}
