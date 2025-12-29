#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

namespace async {

template<WorldPartConcept Part, typename BriefQueue>
auto
ecl_scc_step(
  Part const&     part,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  BriefQueue&     mq,
  vertex_t*       scc_id,
  vertex_t*       ecl_fw_label,
  vertex_t*       ecl_bw_label,
  vertex_t*       active_array,
  BitsAccessor    active,
  BitsAccessor    changed,
  vertex_t        decided_count = 0) -> vertex_t
{
}

} // namespace async
