#pragma once

#include <sys/stat.h>

typedef int index_t;
typedef int vertex_t;

constexpr int scc_id_undecided = -3;
constexpr int scc_id_singular  = -2;
constexpr int scc_id_largest   = -1;

inline off_t
fsize(char const* filename)
{
  struct stat st{};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}
