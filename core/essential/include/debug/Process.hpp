#pragma once

#include <fstream>
#include <unistd.h>

inline auto
get_resident_set_bytes() -> size_t
{
  std::ifstream f{ "/proc/self/statm" };
  if (size_t program_pages, resident_set_pages; f >> program_pages >> resident_set_pages) {
    return resident_set_pages * static_cast<size_t>(sysconf(_SC_PAGESIZE));
  }
  return 0;
}
