#pragma once

#include <briefkasten/aggregators.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/grid_indirection.hpp>
#include <briefkasten/indirection.hpp>
#include <briefkasten/noop_indirection.hpp>
#include <briefkasten/queue_builder.hpp>
#include <kamping/mpi_datatype.hpp>
#include <kaspan/debug/process.hpp>
#include <kaspan/debug/statistic.hpp>
#include <kaspan/memory/accessor/bits.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/scc/allgather_sub_graph.hpp>
#include <kaspan/scc/async/backward_search.hpp>
#include <kaspan/scc/async/color_scc_step.hpp>
#include <kaspan/scc/async/forward_search.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/color_scc_step.hpp>
#include <kaspan/scc/pivot_selection.hpp>
#include <kaspan/scc/tarjan.hpp>
#include <kaspan/scc/trim_1.hpp>
#include <kaspan/util/return_pack.hpp>

#include <algorithm>
#include <cstdio>

namespace kamping {
template<>
struct mpi_type_traits<kaspan::edge>
{
  static constexpr bool has_to_be_committed = false;
  static MPI_Datatype   data_type()
  {
    return kaspan::mpi_edge_t;
  }
};
}

namespace kaspan::async {

template<typename indirection_scheme_t>
auto
make_briefkasten_vertex()
{
  if constexpr (std::same_as<indirection_scheme_t, briefkasten::NoopIndirectionScheme>) {
    return briefkasten::BufferedMessageQueueBuilder<vertex_t>().build();
  } else {
    return briefkasten::IndirectionAdapter{ briefkasten::BufferedMessageQueueBuilder<vertex_t>()
                                              .with_merger(briefkasten::aggregation::EnvelopeSerializationMerger{})
                                              .with_splitter(briefkasten::aggregation::EnvelopeSerializationSplitter<vertex_t>{})
                                              .build(),
                                            indirection_scheme_t{ mpi_basic::comm_world } };
  }
}

template<typename indirection_scheme_t>
auto
make_briefkasten_edge()
{
  if constexpr (std::same_as<indirection_scheme_t, briefkasten::NoopIndirectionScheme>) {
    return briefkasten::BufferedMessageQueueBuilder<edge>().build();
  } else {
    return briefkasten::IndirectionAdapter{ briefkasten::BufferedMessageQueueBuilder<edge>().build(), indirection_scheme_t{ mpi_basic::comm_world } };
  }
}

template<typename indirection_scheme_t, world_part_concept part_t>
void
scc(part_t const& part, index_t const* fw_head, vertex_t const* fw_csr, index_t const* bw_head, vertex_t const* bw_csr, vertex_t* scc_id)
{
  DEBUG_ASSERT_VALID_GRAPH_PART(part, fw_head, fw_csr);
  DEBUG_ASSERT_VALID_GRAPH_PART(part, bw_head, bw_csr);
  KASPAN_STATISTIC_SCOPE("scc");

  auto const n       = part.n;
  auto const local_n = part.local_n();

  vertex_t local_decided = 0;

  KASPAN_STATISTIC_PUSH("trim_1");
  // notice: trim_1_first has a side effect by initializing scc_id with scc_id undecided
  // if trim_1_first is removed one has to initialize scc_id with scc_id_undecided manually!
  auto const [trim_1_decided, max] = trim_1_first(part, fw_head, bw_head, scc_id);
  KASPAN_STATISTIC_ADD("decided_count", mpi_basic::allreduce_single(trim_1_decided, mpi_basic::sum));
  local_decided += trim_1_decided;
  KASPAN_STATISTIC_POP();

  // fallback to tarjan on single rank
  if (mpi_basic::world_size == 1) {
    tarjan(
      part,
      fw_head,
      fw_csr,
      [=](auto const* cbeg, auto const* cend) {
        auto const id = *std::min_element(cbeg, cend);
        std::for_each(cbeg, cend, [=](auto const k) {
          scc_id[k] = id;
        });
      },
      SCC_ID_UNDECIDED_FILTER(local_n, scc_id),
      local_decided);
    return;
  }

  auto const decided_threshold = n - (2 * n / mpi_basic::world_size); // as we only gather fw graph we can only reduce to 2 * local_n
  DEBUG_ASSERT_GE(decided_threshold, 0);

  if (mpi_basic::allreduce_single(local_decided, mpi_basic::sum) < decided_threshold) {
    {
      KASPAN_STATISTIC_SCOPE("forward_backward_search");
      auto const prev_local_decided = local_decided;

      auto pivot        = pivot_selection(max);
      auto bitvector    = make_bits_clean(local_n);
      auto active_array = make_array<vertex_t>(local_n);
      auto vertex_queue = make_briefkasten_vertex<indirection_scheme_t>();

      async::forward_search(part, fw_head, fw_csr, vertex_queue, scc_id, static_cast<bits_accessor>(bitvector), active_array.data(), pivot);
      local_decided += async::backward_search(part, bw_head, bw_csr, vertex_queue, scc_id, static_cast<bits_accessor>(bitvector), active_array.data(), pivot, pivot);

      KASPAN_STATISTIC_ADD("decided_count", mpi_basic::allreduce_single(local_decided - prev_local_decided, mpi_basic::sum));
      KASPAN_STATISTIC_ADD("memory", kaspan::get_resident_set_bytes());
    }
    {
      KASPAN_STATISTIC_SCOPE("trim_1_fw_bw");
      auto const trim_1_decided = trim_1(part, fw_head, fw_csr, bw_head, bw_csr, scc_id);
      local_decided += trim_1_decided;
      KASPAN_STATISTIC_ADD("decided_count", mpi_basic::allreduce_single(trim_1_decided, mpi_basic::sum));
    }
  }

  if (mpi_basic::allreduce_single(local_decided, mpi_basic::sum) < decided_threshold) {
    KASPAN_STATISTIC_SCOPE("color");

    auto colors       = make_array<vertex_t>(local_n);
    auto active_array = make_array<vertex_t>(local_n - local_decided);
    auto active       = make_bits_clean(local_n);
    auto edge_queue   = make_briefkasten_edge<indirection_scheme_t>();

    auto const prev_local_decided = local_decided;

    bool first_iter = true;
    do {
      if (!first_iter) {
        // Reactivate queue for next iteration - requires barrier to prevent race condition
        mpi_basic::barrier();
        edge_queue.reactivate();
      }
      first_iter = false;

      color_scc_init_label(part, colors.data());
      local_decided +=
        async::color_scc_step(part, fw_head, fw_csr, bw_head, bw_csr, edge_queue, scc_id, colors.data(), active_array.data(), static_cast<bits_accessor>(active), local_decided);
    } while (mpi_basic::allreduce_single(local_decided, mpi_basic::sum) < decided_threshold);

    KASPAN_STATISTIC_ADD("decided_count", mpi_basic::allreduce_single(local_decided - prev_local_decided, mpi_basic::sum));
    KASPAN_STATISTIC_ADD("memory", kaspan::get_resident_set_bytes());
  }

  {
    KASPAN_STATISTIC_SCOPE("residual");

    auto [TMP(), sub_ids_inverse, TMP(), sub_n, sub_m, sub_head, sub_csr] =
      allgather_fw_sub_graph(part, local_n - local_decided, fw_head, fw_csr, SCC_ID_UNDECIDED_FILTER(local_n, scc_id));
    DEBUG_ASSERT_VALID_GRAPH(sub_n, sub_m, sub_head, sub_csr);

    if (sub_n) {
      tarjan(sub_n, sub_head, sub_csr, [&](auto const* beg, auto const* end) {
        auto const min_u = sub_ids_inverse[*std::min_element(beg, end)];
        for (auto sub_u : std::span{ beg, end }) {
          auto const u = sub_ids_inverse[sub_u];
          if (part.has_local(u)) {
            scc_id[part.to_local(u)] = min_u;
          }
        }
      });
    }

    KASPAN_STATISTIC_ADD("n", sub_n);
    KASPAN_STATISTIC_ADD("m", sub_m);
    KASPAN_STATISTIC_ADD("memory", kaspan::get_resident_set_bytes());
    KASPAN_STATISTIC_ADD("decided_count", mpi_basic::allreduce_single(local_n - local_decided, mpi_basic::sum));
  }
}

} // namespace kaspan::async
