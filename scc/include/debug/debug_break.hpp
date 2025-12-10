#pragma once

#include <debug/debug.hpp>

#if KASPAN_DEBUG
#if !defined(_MSC_VER)
#include <csignal>
#endif
#endif

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
