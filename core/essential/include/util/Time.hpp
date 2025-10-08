// ReSharper disable CppUnusedIncludeDirective

#pragma once

#include <mpi.h>
#include <sys/time.h>

inline auto
wtime() -> double
{
  timeval t{};
  gettimeofday(&t, nullptr);
  return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_usec) * 1.0e-6;
}

#define BEG_TIME(ID) const auto wtime_##ID##_beg = wtime()
#define END_TIME(ID) const auto wtime_##ID = (wtime() - wtime_##ID##_beg)
#define GET_TIME(ID) wtime_##ID
#define STR_TIME(ID) std::format("{:.9f}s", GET_TIME(ID))


inline double
mpi_global_max_wtime()
{
  auto const local_time      = MPI_Wtime();
  auto       global_max_time = std::numeric_limits<double>::min();
  MPI_Allreduce(&local_time, &global_max_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  return global_max_time;
}
