#include "graph/deserialize.h"
#include "test_util.h"

namespace {
using namespace ka_span;
}

int
main()
{
  constexpr auto success_consume = [](auto) -> ka_void_result {
    return ka_void_result::success();
  };

  ASSERT_EQ(load_graph(success_consume, 1, 4, "this_file_does_not_exist").error_or(OK), FILESYSTEM_ERROR);
}
