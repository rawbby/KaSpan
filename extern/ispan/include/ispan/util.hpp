#pragma once

#include <kaspan/mpi_basic/mpi_basic.hpp>
#include <kaspan/scc/base.hpp>
#include <sys/stat.h>

using depth_t = kaspan::i16;

constexpr inline auto mpi_depth_t = kaspan::mpi_basic::type<depth_t>;

constexpr inline depth_t depth_unset = -1;

constexpr inline kaspan::vertex_t scc_id_largest = kaspan::scc_id_undecided - 2;

inline off_t
fsize(char const* filename)
{
  struct stat st
  {};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}
