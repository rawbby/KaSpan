#pragma once

#include <kaspan/util/pp.hpp>

#if defined(KASPAN_MEMCHECK)
#if KASPAN_MEMCHECK != 0
#undef KASPAN_MEMCHECK
#define KASPAN_MEMCHECK 1
#endif
#else
#define KASPAN_MEMCHECK 0
#endif

#if defined(KASPAN_CALLGRIND)
#if KASPAN_CALLGRIND != 0
#undef KASPAN_CALLGRIND
#define KASPAN_CALLGRIND 1
#endif
#else
#define KASPAN_CALLGRIND 0
#endif

#if OR(KASPAN_MEMCHECK, KASPAN_CALLGRIND)
#include <valgrind/callgrind.h>
#include <valgrind/memcheck.h>
#include <valgrind/valgrind.h>
#endif

// clang-format off

#define KASPAN_MEMCHECK_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed) IF(KASPAN_MEMCHECK, VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed))
#define KASPAN_MEMCHECK_FREELIKE_BLOCK(_addr, _rzB)                       IF(KASPAN_MEMCHECK, VALGRIND_FREELIKE_BLOCK(_addr, _rzB))

#define KASPAN_MEMCHECK_MAKE_MEM_NOACCESS(_addr, _len)                    IF(KASPAN_MEMCHECK, VALGRIND_MAKE_MEM_NOACCESS(_addr, _len))
#define KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(_addr, _len)                   IF(KASPAN_MEMCHECK, VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len))
#define KASPAN_MEMCHECK_MAKE_MEM_DEFINED(_addr, _len)                     IF(KASPAN_MEMCHECK, VALGRIND_MAKE_MEM_DEFINED(_addr, _len))

#define KASPAN_MEMCHECK_MAKE_VALUE_NOACCESS(_val)                         KASPAN_MEMCHECK_MAKE_MEM_NOACCESS(&_val, sizeof(_val))
#define KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(_val)                        KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(&_val, sizeof(_val))
#define KASPAN_MEMCHECK_MAKE_VALUE_DEFINED(_val)                          KASPAN_MEMCHECK_MAKE_MEM_DEFINED(&_val, sizeof(_val))

#define KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(_addr, _len)                 IF(KASPAN_MEMCHECK, VALGRIND_CHECK_MEM_IS_DEFINED(_addr, _len))
#define KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(_addr, _len)             IF(KASPAN_MEMCHECK, VALGRIND_CHECK_MEM_IS_ADDRESSABLE(_addr, _len))
#define KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(_val)                      IF(KASPAN_MEMCHECK, VALGRIND_CHECK_VALUE_IS_DEFINED(_val))

#define KASPAN_MEMCHECK_PRINTF(FORMAT, ...)                               IF(KASPAN_MEMCHECK, VALGRIND_PRINTF(FORMAT __VA_OPT__(, __VA_ARGS__)))
#define KASPAN_MEMCHECK_PRINTF_BACKTRACE(FORMAT, ...)                     IF(KASPAN_MEMCHECK, VALGRIND_PRINTF_BACKTRACE(FORMAT __VA_OPT__(, __VA_ARGS__)))
#define KASPAN_MEMCHECK_BACKTRACE()                                       IF(KASPAN_MEMCHECK, VALGRIND_PRINTF_BACKTRACE("[BACKTRACE] user requested backtrace"))

#define KASPAN_CALLGRIND_START_INSTRUMENTATION() IF(KASPAN_CALLGRIND, CALLGRIND_START_INSTRUMENTATION)
#define KASPAN_CALLGRIND_STOP_INSTRUMENTATION()  IF(KASPAN_CALLGRIND, CALLGRIND_STOP_INSTRUMENTATION)
#define KASPAN_CALLGRIND_TOGGLE_COLLECT()        IF(KASPAN_CALLGRIND, CALLGRIND_TOGGLE_COLLECT)

// clang-format on
