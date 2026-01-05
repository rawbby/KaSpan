#pragma once

#include <concepts>
#include <kaspan/debug/assert.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <mpi.h>
#include <numeric> // std::accumulate

namespace kaspan::mpi_basic {

/**
 * @brief In-place, zero-extra-memory partition of a send buffer into per-rank buckets.
 *
 * @tparam T  Item type stored in send_buffer.
 * @tparam fn_t Callable with signature i32(T const&), returning the destination rank.
 */
template<typename T, typename fn_t>
  requires(std::convertible_to<std::invoke_result_t<fn_t, T const&>, i32>)
void
inplace_partition_by_rank(T* send_buffer, MPI_Count const* send_counts, MPI_Aint* send_displs, fn_t&& rank_of)
{
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);
  if (send_buffer == nullptr) {
    DEBUG_ASSERT_EQ(std::accumulate(send_counts, send_counts + world_size, static_cast<MPI_Count>(0)), 0);
  }

  // Initialize send_displs as element-index prefix sums.
  send_displs[0] = 0;
  for (i32 r = 1; r < world_size; ++r) {
    send_displs[r] = send_displs[r - 1] + static_cast<MPI_Aint>(send_counts[r - 1]);
  }

  MPI_Aint ordered_items = 0;
  for (i32 rank = 0; rank < world_size; ++rank) {
    auto const rank_beg = ordered_items;
    auto const rank_end = rank_beg + static_cast<MPI_Aint>(send_counts[rank]);

    ordered_items = send_displs[rank];

    send_displs[rank] = rank_beg;

    while (ordered_items < rank_end) {
      auto const target_rank = static_cast<i32>(rank_of(send_buffer[ordered_items]));
      DEBUG_ASSERT_GE(target_rank, rank);
      DEBUG_ASSERT_LT(target_rank, world_size);

      if (target_rank == rank) {
        ++ordered_items;
      } else {
        auto const target_rank_ordered_end = send_displs[target_rank]++;
        std::swap(send_buffer[ordered_items], send_buffer[target_rank_ordered_end]);
      }
    }
  }
}

} // namespace kaspan::mpi_basic
