#pragma once

#include <csignal>
#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/valgrind.hpp>

namespace kaspan {

inline void
debug_break() noexcept
{
  KASPAN_VALGRIND_PRINTF_BACKTRACE("[BREAK] break point triggered - backtrace requested");

#if KASPAN_DEBUG
#if defined(_MSC_VER)
  __debugbreak();
#else
  std::raise(SIGTRAP); // NOLINT(*-err33-c)
#endif
#endif
}

} // namespace kaspan
