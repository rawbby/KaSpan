#pragma once

#include <algorithm>
#include <sys/stat.h>

typedef int          index_t;
typedef int          vertex_t;

inline off_t
fsize(const char* filename)
{
  struct stat st{};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}
