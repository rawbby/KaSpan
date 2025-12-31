#pragma once

#include <memory/accessor/bits_accessor.hpp>
#include <memory/accessor/stack_accessor.hpp>
#include <scc/base.hpp>
#include <scc/graph.hpp>

namespace async {

template<WorldPartConcept Part>
void
ecl_scc_init_label(
  Part const& part,
  vertex_t*   ecl_fw_label,
  vertex_t*   ecl_bw_label)
{
  auto const local_n = part.local_n();
  for (vertex_t k = 0; k < local_n; ++k) {
    auto const u    = part.to_global(k);
    ecl_fw_label[k] = u;
    ecl_bw_label[k] = u;
  }
}

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
  auto const local_n = part.local_n();

  auto propagate = [&](index_t const* head, vertex_t const* csr, vertex_t* labels) {
    auto active_stack = StackAccessor<vertex_t>{ active_array };

    auto on_message = [&](auto env) {
      for (auto edge : env.message) {

        auto const u     = edge.u;
        auto const label = edge.v;
        DEBUG_ASSERT(part.has_local(u));
        auto const k = part.to_local(u);
        if (label < labels[k] and scc_id[k] == scc_id_undecided) {
          labels[k] = label;
          changed.set(k);
          if (!active.get(k)) {
            active.set(k);
            active_stack.push(k);
          }
        }
      }
    };

    active.fill_cmp(local_n, scc_id, scc_id_undecided);
    std::memcpy(changed.data(), active.data(), (local_n + 7) >> 3);
    active.for_each(local_n, [&](auto k) {
      active_stack.push(k);
    });

    while (true) {
      while (!active_stack.empty()) {
        auto const k = active_stack.back();
        active_stack.pop();
        active.unset(k);

        auto const label_k = labels[k];
        for (auto v : csr_range(head, csr, k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (label_k < labels[l] and scc_id[l] == scc_id_undecided) {
              labels[l] = label_k;
              changed.set(l);
              if (!active.get(l)) {
                active.set(l);
                active_stack.push(l);
              }
            }
          }
        }
      }

      changed.for_each(local_n, [&](auto k) {
        changed.unset(k);
        auto const label_k = labels[k];
        for (auto v : csr_range(head, csr, k)) {
          if (not part.has_local(v) and label_k < v) {
            mq.post_message_blocking(Edge{ v, label_k }, part.world_rank_of(v), on_message);
          }
        }
      });

      mq.poll_throttled(on_message);
      if (active_stack.empty() and mq.terminate(on_message)) {
        break;
      }
    }
  };

  mpi_basic_barrier();
  mq.reactivate();
  mpi_basic_barrier();

  propagate(fw_head, fw_csr, ecl_fw_label);

  mpi_basic_barrier();
  mq.reactivate();
  mpi_basic_barrier();

  propagate(bw_head, bw_csr, ecl_bw_label);

  mpi_basic_barrier();
  mq.reactivate();
  mpi_basic_barrier();

  vertex_t local_decided_count = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    if (scc_id[k] == scc_id_undecided) {
      if (ecl_fw_label[k] == ecl_bw_label[k]) {
        scc_id[k] = ecl_fw_label[k];
        ++local_decided_count;
      }
    }
  }

  ASSERT_GE(local_decided_count, 0);
  return local_decided_count;
}

} // namespace async
