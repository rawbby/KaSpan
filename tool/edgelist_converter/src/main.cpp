#include <kaspan/scc/adapter/external_edgelist.hpp>
#include <kaspan/util/arg_parse.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/result.hpp>

#include <cstdio>
#include <cstdlib>
#include <print>

using namespace kaspan;

[[noreturn]] void
usage(int /* argc */, char** argv)
{
  std::println("usage: {} --edge_list <edge_list> [--memory <memory>]", argv[0]);
  std::exit(1);
}

int
main(int argc, char** argv)
{
  auto const* const edge_list = arg_select_str(argc, argv, "--edge_list", usage);
  auto const        memory    = arg_select_default_int<u64>(argc, argv, "--memory", static_cast<u64>(8) * 1024 * 1024 * 1024);
  ASSERT_TRY(convert_graph(edge_list, memory));
}
