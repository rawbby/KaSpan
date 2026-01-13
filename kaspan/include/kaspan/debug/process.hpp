#pragma once

#include <kaspan/util/integral_cast.hpp>

#include <fstream>
#include <unistd.h>

namespace kaspan {

inline auto
get_resident_set_bytes() -> size_t
{
  std::ifstream f{ "/proc/self/statm" };

  size_t program_pages      = 0;
  size_t resident_set_pages = 0;
  if (f >> program_pages >> resident_set_pages) {
    return resident_set_pages * integral_cast<size_t>(sysconf(_SC_PAGESIZE));
  }
  return 0;
}

} // namespace kaspan
