#pragma once

#include <algorithm>
#include <memory/accessor/bits_accessor.hpp>
#include <memory/accessor/stack_accessor.hpp>
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
  vertex_t*       active_array,
  vertex_t        root,
  vertex_t        id) -> vertex_t
{
  auto const local_n       = part.local_n();
  auto       active_stack  = StackAccessor<vertex_t>{ active_array };
  vertex_t   decided_count = 0;
  vertex_t   min_u         = part.n;

  auto on_message = [&](auto env) {
    for (auto v : env.message) {
      DEBUG_ASSERT(part.has_local(v));
      auto const k = part.to_local(v);
      if (fw_reached.get(k) and scc_id[k] == scc_id_undecided) {
        scc_id[k] = id;
        min_u     = std::min(min_u, v);
        ++decided_count;
        active_stack.push(k);
      }
    }
  };

  if (part.has_local(root)) {
    auto const k = part.to_local(root);
    if (fw_reached.get(k) and scc_id[k] == scc_id_undecided) {
      scc_id[k] = id;
      min_u     = std::min(min_u, root);
      ++decided_count;
      active_stack.push(k);
    }
  }

  mpi_basic::barrier();
  mq.reactivate();

  while (true) {
    while (not active_stack.empty()) {
      auto const k = active_stack.back();
      active_stack.pop();

      for (auto v : csr_range(bw_head, bw_csr, k)) {
        if (part.has_local(v)) {
          auto const l = part.to_local(v);
          if (fw_reached.get(l) and scc_id[l] == scc_id_undecided) {
            scc_id[l] = id;
            min_u     = std::min(min_u, v);
            ++decided_count;
            active_stack.push(l);
          }
        } else {
          mq.post_message_blocking(v, part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }

    mq.poll_throttled(on_message);
    if (active_stack.empty() and mq.terminate(on_message)) {
      break;
    }
  }

  // normalise scc_id to minimum vertex in scc
  min_u = mpi_basic::allreduce_single(min_u, mpi_basic::min);
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == id) {
      scc_id[k] = min_u;
    }
  }

  return decided_count;
}

} // namespace async
