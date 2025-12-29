#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
auto
backward_search(
  Part const&     part,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  BriefQueue&     mq,
  vertex_t*       scc_id,
  BitsAccessor    fw_reached,
  vertex_t        root,
  vertex_t        id) -> vertex_t
{
  return 0;
}

} // namespace async
