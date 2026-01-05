#pragma once

#include <kaspan/util/pp.hpp>
#if defined(KASPAN_VALGRIND) && KASPAN_VALGRIND != 0
#include <valgrind/memcheck.h>
#include <valgrind/valgrind.h>
#endif

namespace kaspan {

#if defined(KASPAN_VALGRIND)
#if KASPAN_VALGRIND != 0
#undef KASPAN_VALGRIND
#define KASPAN_VALGRIND 1
#endif
#else
#define KASPAN_VALGRIND 0
#endif

#if KASPAN_VALGRIND

#define KASPAN_VALGRIND_RUNNING_ON_VALGRIND RUNNING_ON_VALGRIND
#define KASPAN_VALGRIND_MAKE_MEM_NOACCESS(_addr, _len) VALGRIND_MAKE_MEM_NOACCESS(_addr, _len)
#define KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len) VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len)
#define KASPAN_VALGRIND_MAKE_MEM_DEFINED(_addr, _len) VALGRIND_MAKE_MEM_DEFINED(_addr, _len)
#define KASPAN_VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed) VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed)
#define KASPAN_VALGRIND_FREELIKE_BLOCK(_addr, _rzB) VALGRIND_FREELIKE_BLOCK(_addr, _rzB)
#define KASPAN_VALGRIND_PRINTF(...) VALGRIND_PRINTF(__VA_ARGS__)
#define KASPAN_VALGRIND_PRINTF_BACKTRACE(...) VALGRIND_PRINTF_BACKTRACE(__VA_ARGS__)
#else
enum
{
  KASPAN_VALGRIND_RUNNING_ON_VALGRIND = 0
};
#define KASPAN_VALGRIND_MAKE_MEM_NOACCESS(_addr, _len) (void)0
#define KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len) (void)0
#define KASPAN_VALGRIND_MAKE_MEM_DEFINED(_addr, _len) (void)0
#define KASPAN_VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed) (void)0
#define KASPAN_VALGRIND_FREELIKE_BLOCK(_addr, _rzB) (void)0
#define KASPAN_VALGRIND_PRINTF(...) (void)0
#define KASPAN_VALGRIND_PRINTF_BACKTRACE(...) (void)0
#endif

} // namespace kaspan
