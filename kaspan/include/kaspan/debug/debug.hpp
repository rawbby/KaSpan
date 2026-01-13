#pragma once

#include <kaspan/util/pp.hpp>

namespace kaspan {

/* This header defines `KASPAN_NDEBUG` and `KASPAN_DEBUG`.
 * Other than marcos like `NDEBUG` or `DEBUG`, `KASPAN_NDEBUG` and `KASPAN_DEBUG`
 * will always be defined and hold either a 0 or 1, making it compatible
 * to `pp.hpp`'s macro utilities.
 *
 * Example Usage:
 * ```cpp
 * #include <kaspan/debug/debug.hpp>
 * #include <kaspan/util/pp.hpp>
 * constexpr auto some_global = IF(KASPAN_NDEBUG, 1, 0);
 * ```
 */

// if defined ensure 0 or 1
#if defined(KASPAN_NDEBUG)
#if KASPAN_NDEBUG != 0
#define KASPAN_NDEBUG 1
#endif
#endif

// if defined ensure 0 or 1
#if defined(KASPAN_DEBUG)
#if KASPAN_DEBUG != 0
#define KASPAN_DEBUG 1
#endif
#endif

// fallback: derive from compiler
#if !defined(KASPAN_DEBUG) && !defined(KASPAN_NDEBUG)
#if defined(DEBUG) || defined(_DEBUG)
#define KASPAN_DEBUG 1
#endif
#if defined(NDEBUG)
#define KASPAN_NDEBUG 1
#endif
#endif
// now at least one is set!

// resolve implications
#if defined(KASPAN_NDEBUG) && !defined(KASPAN_DEBUG)
#define KASPAN_DEBUG NOT(KASPAN_NDEBUG)
#endif
#if defined(KASPAN_DEBUG) && !defined(KASPAN_NDEBUG)
#define KASPAN_NDEBUG NOT(KASPAN_DEBUG)
#endif
// now both are set

// final fallback: in case prior
// fallback had no hints (unusual)
#if !defined(KASPAN_NDEBUG) && !defined(KASPAN_DEBUG)
#define KASPAN_NDEBUG 1
#define KASPAN_DEBUG 0
#endif

// sanity check
#if XNOR(KASPAN_DEBUG, KASPAN_NDEBUG)
#error "Can't interpret mixed debug hints. Ensure to define KASPAN_NDEBUG not and KASPAN_DEBUG."
#endif

} // namespace kaspan
