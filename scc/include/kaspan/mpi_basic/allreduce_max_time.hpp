#pragma once

#include <chrono>
#include <kaspan/mpi_basic/allreduce_single.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <mpi.h>

namespace kaspan::mpi_basic {

/**
 * @brief Get the maximum current time across all ranks.
 */
inline auto
allreduce_max_time() -> u64
{
  using namespace std::chrono;
  auto const now          = steady_clock::now();
  auto const timestamp    = now.time_since_epoch();
  auto const timestamp_ns = duration_cast<nanoseconds>(timestamp);
  return allreduce_single(static_cast<u64>(timestamp_ns.count()), MPI_MAX);
}

} // namespace kaspan::mpi_basic
