#pragma once

#include <algorithm>
#include <sys/stat.h>
#include <sys/time.h>

typedef int index_t;
typedef int vertex_t;

inline off_t
fsize(char const* filename)
{
  struct stat st{};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}

inline double
wtime()
{
  double  time[2];
  timeval time1{};
  gettimeofday(&time1, nullptr);
  time[0] = time1.tv_sec;
  time[1] = time1.tv_usec;
  return time[0] + time[1] * 1.0e-6;
}
