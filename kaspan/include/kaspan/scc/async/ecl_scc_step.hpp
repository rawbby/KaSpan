#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>

namespace kaspan {

namespace async {

template<world_part_concept Part>
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

template<world_part_concept Part,
         typename brief_queue_t>
auto
ecl_scc_step(
  bidi_graph_part_view<Part> graph,
  brief_queue_t&             mq,
  vertex_t*                  scc_id,
  vertex_t*                  ecl_fw_label,
  vertex_t*                  ecl_bw_label,
  vertex_t*                  fw_active_array,
  vertex_t*                  bw_active_array,
  bits_accessor              fw_active,
  bits_accessor              bw_active,
  bits_accessor              fw_changed,
  bits_accessor              bw_changed,
  vertex_t                   decided_count = 0) -> vertex_t
{
  auto const& part    = *graph.part;
  auto const  local_n = part.local_n();

  auto fw_active_stack = view_stack<vertex_t>(fw_active_array, local_n);
  auto bw_active_stack = view_stack<vertex_t>(bw_active_array, local_n);

  auto on_fw_message = [&](auto env) {
    for (auto edge : env.message) {
      auto const u     = edge.u;
      auto const label = edge.v;
      DEBUG_ASSERT(part.has_local(u));
      auto const k = part.to_local(u);
      if (scc_id[k] == scc_id_undecided and label < ecl_fw_label[k]) {
        ecl_fw_label[k] = label;
        fw_changed.set(k);
        if (!fw_active.get(k)) {
          fw_active.set(k);
          fw_active_stack.push(k);
        }
      }
    }
  };

  auto on_bw_message = [&](auto env) {
    for (auto edge : env.message) {
      auto const u     = edge.u;
      auto const label = edge.v;
      DEBUG_ASSERT(part.has_local(u));
      auto const k = part.to_local(u);
      if (scc_id[k] == scc_id_undecided and label < ecl_bw_label[k]) {
        ecl_bw_label[k] = label;
        bw_changed.set(k);
        if (!bw_active.get(k)) {
          bw_active.set(k);
          bw_active_stack.push(k);
        }
      }
    }
  };

  fw_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
  std::memcpy(fw_changed.data(), fw_active.data(), (local_n + 7) >> 3);
  fw_active.for_each(local_n, [&](auto k) {
    fw_active_stack.push(k);
  });

  bw_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
  std::memcpy(bw_changed.data(), bw_active.data(), (local_n + 7) >> 3);
  bw_active.for_each(local_n, [&](auto k) {
    bw_active_stack.push(k);
  });

  mpi_basic::barrier();
  mq.reactivate();

  while (true) {
    // 1 & 2. local fw saturation & read briefkasten into bw buffer
    while (true) {
      while (!fw_active_stack.empty()) {
        auto const k = fw_active_stack.back();
        fw_active_stack.pop();
        fw_active.unset(k);

        auto const label_k = ecl_fw_label[k];
        for (auto v : graph.fw_csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and label_k < ecl_fw_label[l]) {
              ecl_fw_label[l] = label_k;
              fw_changed.set(l);
              if (!fw_active.get(l)) {
                fw_active.set(l);
                fw_active_stack.push(l);
              }
            }
          }
        }
        mq.poll_throttled(on_bw_message);
      }

      mq.poll_throttled(on_bw_message);
      if (fw_active_stack.empty() and mq.terminate(on_bw_message)) {
        break;
      }
    }

    // 3. barrier + reactivate and push fw borders
    mpi_basic::barrier();
    mq.reactivate();
    bool fw_pushed = false;
    fw_changed.for_each(local_n, [&](auto k) {
      fw_pushed = true;
      fw_changed.unset(k);
      auto const label_k = ecl_fw_label[k];
      for (auto v : graph.fw_csr_range(k)) {
        if (not part.has_local(v) and label_k < v) {
          mq.post_message_blocking(edge{ v, label_k }, part.world_rank_of(v), on_fw_message);
        }
      }
    });

    // 4 & 5. local bw saturation & read briefkasten into fw buffer
    while (true) {
      while (!bw_active_stack.empty()) {
        auto const k = bw_active_stack.back();
        bw_active_stack.pop();
        bw_active.unset(k);

        auto const label_k = ecl_bw_label[k];
        for (auto v : graph.bw_csr_range(k)) {
          if (part.has_local(v)) {
            auto const l = part.to_local(v);
            if (scc_id[l] == scc_id_undecided and label_k < ecl_bw_label[l]) {
              ecl_bw_label[l] = label_k;
              bw_changed.set(l);
              if (!bw_active.get(l)) {
                bw_active.set(l);
                bw_active_stack.push(l);
              }
            }
          }
        }
        mq.poll_throttled(on_fw_message);
      }

      mq.poll_throttled(on_fw_message);
      if (bw_active_stack.empty() and mq.terminate(on_fw_message)) {
        break;
      }
    }

    // 6. barrier + reactivate and push bw borders
    mpi_basic::barrier();
    mq.reactivate();
    bool bw_pushed = false;
    bw_changed.for_each(local_n, [&](auto k) {
      bw_pushed = true;
      bw_changed.unset(k);
      auto const label_k = ecl_bw_label[k];
      for (auto v : graph.bw_csr_range(k)) {
        if (not part.has_local(v) and label_k < v) {
          mq.post_message_blocking(edge{ v, label_k }, part.world_rank_of(v), on_bw_message);
        }
      }
    });

    if (!mpi_basic::allreduce_single(fw_pushed or bw_pushed, mpi_basic::lor)) {
      break;
    }
  }

  mpi_basic::barrier();
  mq.reactivate();

  vertex_t local_decided_count = 0;
  fw_active.set_each(local_n, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
  fw_active.for_each(local_n, [&](auto k) {
    if (ecl_fw_label[k] == ecl_bw_label[k]) {
      scc_id[k] = ecl_fw_label[k];
      ++local_decided_count;
    }
  });

  ASSERT_GE(local_decided_count, 0);
  return local_decided_count;
}

} // namespace async

} // namespace kaspan
