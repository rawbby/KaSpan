#include <buffer/Buffer.hpp>
#include <graph/ConvertGraph.hpp>
#include <graph/GraphPart.hpp>
#include <scc/Scc.hpp>
#include <test/Assert.hpp>

auto
main(int argc, char** argv) -> int
{
  ASSERT(argc >= 2, "Usage: ./graph_converter <edgelist> [<memory>]");
  ASSERT(argc <= 3, "Usage: ./graph_converter <edgelist> [<memory>]");

  if (argc == 3) {
    size_t memory = 0;
    ASSERT(std::from_chars(argv[2], argv[2] + std::strlen(argv[2]), memory).ec == std::errc());
    ASSERT_TRY(convert_graph(argv[1], memory));
  } else {
    constexpr size_t memory = static_cast<size_t>(8) * 1024 * 1024 * 1024;
    ASSERT_TRY(convert_graph(argv[1], memory));
  }
}
