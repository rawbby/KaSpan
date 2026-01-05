#pragma once

#include <csignal>
#include <kaspan/debug/debug.hpp>

namespace kaspan {

inline void
debug_break() noexcept
{
#if KASPAN_DEBUG
#if defined(_MSC_VER)
  __debugbreak();
#else
  std::raise(SIGTRAP); // NOLINT(*-err33-c)
#endif
#endif
}

} // namespace kaspan
