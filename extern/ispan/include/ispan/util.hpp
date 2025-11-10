#pragma once

#include <scc/base.hpp>
#include <sys/stat.h>
#include <util/mpi_basic.hpp>

using depth_t = i16;

constexpr inline auto mpi_depth_t = mpi_basic_type<depth_t>;

constexpr inline depth_t depth_unset = -1;

constexpr inline vertex_t scc_id_largest = scc_id_undecided - 2;

inline off_t
fsize(char const* filename)
{
  struct stat st{};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}
