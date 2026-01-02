#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <memory/accessor/stack_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

namespace async {

// template<WorldPartConcept Part>
// void
// color_scc_init_label(
//   Part const& part,
//   vertex_t*   colors)
// {
//   auto const local_n = part.local_n();
//   for (vertex_t k = 0; k < local_n; ++k) {
//     colors[k] = part.to_global(k);
//   }
// }

template<WorldPartConcept Part, typename BriefQueue>
auto
color_scc_step(
  Part const&     part,
  index_t const*  fw_head,
  vertex_t const* fw_csr,
  index_t const*  bw_head,
  vertex_t const* bw_csr,
  BriefQueue&     mq,
  vertex_t*       scc_id,
  vertex_t*       colors,
  vertex_t*       active_array,
  BitsAccessor    active,
  vertex_t        decided_count = 0) -> vertex_t
{
  auto const local_n = part.local_n();

#if KASPAN_DEBUG
  // Validate decided_count is consistent with scc_id
  vertex_t actual_decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] != scc_id_undecided) {
      ++actual_decided_count;
    }
  }
  DEBUG_ASSERT_EQ(actual_decided_count, decided_count);
#endif

  auto active_stack = StackAccessor<vertex_t>{ active_array };

  // Phase 1: Forward Color Propagation
  {
    auto on_message = [&](auto env) {
      for (auto const& edge : env.message) {
        auto const u     = edge.u;
        auto const label = edge.v;
        DEBUG_ASSERT(part.has_local(u));
        auto const k = part.to_local(u);
        if (scc_id[k] == scc_id_undecided and label < colors[k]) {
          colors[k] = label;
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
          }
        }
      }
    };

    active.fill_cmp(local_n, scc_id, scc_id_undecided);
    active.for_each(local_n, [&](auto&& k) {
      active_stack.push(k);
    });

    mpi_basic::barrier();
    mq.reactivate();

    while (true) {
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();
        active.unset(k);

        auto const label = colors[k];
        for (auto v : csr_range(fw_head, fw_csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and label < colors[l]) {
              colors[l] = label;
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          } else {
            mq.post_message_blocking(Edge{ v, label }, part.world_rank_of(v), on_message);
          }
        }
        mq.poll_throttled(on_message);
      }

      mq.poll_throttled(on_message);
      if (active_stack.empty() and mq.terminate(on_message)) {
        break;
      }
    }
  }

  // Phase 2: Multi-pivot Backward Search
  vertex_t local_decided_count = 0;
  {
    auto on_message = [&](auto env) {
      for (auto const& edge : env.message) {
        auto const u     = edge.u;
        auto const pivot = edge.v;
        DEBUG_ASSERT(part.has_local(u));
        auto const k = part.to_local(u);
        if (scc_id[k] == scc_id_undecided and colors[k] == pivot) {
          scc_id[k] = pivot;
          ++local_decided_count;
          if (not active.get(k)) {
            active.set(k);
            active_stack.push(k);
          }
        }
      }
    };

    DEBUG_ASSERT(active_stack.empty());
    active.fill_cmp(local_n, scc_id, scc_id_undecided);
    active.for_each(local_n, [&](auto k) {
      if (colors[k] == part.to_global(k)) {
        scc_id[k] = colors[k];
        ++local_decided_count;
        active_stack.push(k);
      } else {
        active.unset(k);
      }
    });

    mpi_basic::barrier();
    mq.reactivate();

    while (true) {
      while (not active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();
        active.unset(k);

        auto const pivot = colors[k];
        for (auto v : csr_range(bw_head, bw_csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and colors[l] == pivot) {
              scc_id[l] = pivot;
              ++local_decided_count;
              if (not active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          } else {
            mq.post_message_blocking(Edge{ v, pivot }, part.world_rank_of(v), on_message);
          }
        }
        mq.poll_throttled(on_message);
      }

      mq.poll_throttled(on_message);
      if (active_stack.empty() and mq.terminate(on_message)) {
        break;
      }
    }
  }

  return local_decided_count;
}

} // namespace async
