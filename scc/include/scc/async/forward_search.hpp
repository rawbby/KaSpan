#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>
#include <vector>

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
  auto const local_n = part.local_n();
  std::vector<vertex_t> active_stack;

  auto on_message = [&](auto env) {
    for (auto v : env.message) {
      DEBUG_ASSERT(part.has_local(v));
      auto const k = part.to_local(v);
      if (not fw_reached.get(k) and scc_id[k] == scc_id_undecided) {
        fw_reached.set(k);
        active_stack.push_back(k);
      }
    }
  };

  if (part.has_local(root)) {
    auto const k = part.to_local(root);
    if (not fw_reached.get(k) and scc_id[k] == scc_id_undecided) {
      fw_reached.set(k);
      active_stack.push_back(k);
    }
  }

  mpi_basic_barrier();
  mq.reactivate();

  while (true) {
    while (not active_stack.empty()) {
      auto const k = active_stack.back();
      active_stack.pop_back();

      for (auto v : csr_range(fw_head, fw_csr, k)) {
        if (part.has_local(v)) {
          auto const l = part.to_local(v);
          if (not fw_reached.get(l) and scc_id[l] == scc_id_undecided) {
            fw_reached.set(l);
            active_stack.push_back(l);
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

  return 0;
}

} // namespace async
