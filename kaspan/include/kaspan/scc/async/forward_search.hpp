#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>
#include <kaspan/util/mpi_basic.hpp>

namespace kaspan::async {

template<part_concept Part,
         typename brief_queue_t>
auto
forward_search(
  graph_part_view<Part> graph,
  brief_queue_t&        mq,
  vertex_t const*       scc_id,
  u64*                  fw_reached_storage,
  vertex_t*             active_array,
  vertex_t              root) -> vertex_t
{
  auto const& part         = *graph.part;
  auto const  local_n      = part.local_n();
  auto        fw_reached   = view_bits(fw_reached_storage, local_n);
  auto        active_stack = view_stack<vertex_t>(active_array, local_n);

  auto on_message = [&](auto env) {
    for (auto v : env.message) {
      DEBUG_ASSERT(part.has_local(v));
      auto const k = part.to_local(v);
      if (!fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
        fw_reached.set(k);
        active_stack.push(k);
      }
    }
  };

  if (part.has_local(root)) {
    auto const k = part.to_local(root);
    if (!fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
      fw_reached.set(k);
      active_stack.push(k);
    }
  }

  mpi_basic::barrier();
  mq.reactivate();

  while (true) {
    while (!active_stack.empty()) {
      auto const k = active_stack.back();
      active_stack.pop();

      for (auto v : graph.csr_range(k)) {
        if (part.has_local(v)) {
          auto const l = part.to_local(v);
          if (!fw_reached.get(l) && scc_id[l] == scc_id_undecided) {
            fw_reached.set(l);
            active_stack.push(l);
          }
        } else {
          mq.post_message_blocking(v, part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }

    mq.poll_throttled(on_message);
    if (active_stack.empty() && mq.terminate(on_message)) {
      break;
    }
  }

  return 0;
}

} // namespace kaspan::async
