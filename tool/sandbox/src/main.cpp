#include "graph/deserialize.h"

namespace {
using namespace ka_span;
}

template<is_index GV, is_index GI, is_index LI>
ka_void_result
consume(distributed_graph<GV, GI, LI> graph)
{
  return ka_void_result::success();
}

int
main()
{
  return load_graph(
           [](auto graph) -> ka_void_result {
             return consume(graph);
           },
           2,
           4,
           "")
    .error_or(OK);
}
