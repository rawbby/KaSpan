#include <debug/assert.hpp>
#include <memory/buffer.hpp>
#include <scc/adapter/external_edgelist.hpp>
#include <scc/adapter/manifest.hpp>
#include <scc/scc.hpp>
#include <util/arg_parse.hpp>

[[noreturn]] void
usage(int /* argc */, char** argv)
{
  std::cout
    << "usage: " << argv[0]
    << " --edge_list <edge_list>"
    << " [--memory <memory>]"
    << std::endl;
  std::exit(1);
}

int
main(int argc, char** argv)
{
  auto const edge_list = arg_select_str(argc, argv, "--edge_list", usage);
  auto const memory    = arg_select_default_int<u64>(argc, argv, "--memory", static_cast<u64>(8) * 1024 * 1024 * 1024);
  ASSERT_TRY(convert_graph(edge_list, memory));
}
