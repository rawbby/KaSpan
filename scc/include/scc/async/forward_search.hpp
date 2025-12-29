#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
auto
forward_search(
  Part const&     part,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  BriefQueue&     mq,
  vertex_t const* scc_id,
  BitsAccessor    fw_reached,
  vertex_t        root) -> vertex_t
{
}

} // namespace async
