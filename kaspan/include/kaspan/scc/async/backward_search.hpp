#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>

namespace kaspan::async {

template<part_view_concept Part,
         typename brief_queue_t>
auto
backward_search(
  bidi_graph_part_view<Part> graph,
  brief_queue_t&             mq,
  vertex_t*                  scc_id,
  u64*                       fw_reached_storage,
  vector<vertex_t>&          active,
  vertex_t                   root) -> vertex_t
{
  auto     fw_reached    = view_bits(fw_reached_storage, graph.part.local_n());
  vertex_t decided_count = 0;

  DEBUG_ASSERT(active.empty());

  auto on_message = [&](auto env) {
    for (auto v : env.message) {
      DEBUG_ASSERT(graph.part.has_local(v));
      auto const k = graph.part.to_local(v);
      if (fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
        scc_id[k] = root;
        ++decided_count;
        active.push_back(k);
      }
    }
  };

  if (graph.part.has_local(root)) {
    auto const k = graph.part.to_local(root);
    if (fw_reached.get(k) && scc_id[k] == scc_id_undecided) {
      scc_id[k] = root;
      ++decided_count;
      active.push_back(k);
    }
  }

  mq.reactivate();

  do {
    while (!active.empty()) {
      auto const k = active.back();
      active.pop_back();

      for (auto v : graph.bw_csr_range(k)) {
        if (graph.part.has_local(v)) {
          auto const l = graph.part.to_local(v);
          if (fw_reached.get(l) && scc_id[l] == scc_id_undecided) {
            scc_id[l] = root;
            ++decided_count;
            active.push_back(l);
          }
        } else {
          mq.post_message_blocking(v, graph.part.world_rank_of(v), on_message);
        }
      }
      mq.poll_throttled(on_message);
    }
  } while(!mq.terminate(on_message));

  DEBUG_ASSERT(active.empty());

  return decided_count;
}

} // namespace kaspan::async
