#pragma once
#include <kaspan/util/pp.hpp>
#if defined(KASPAN_VALGRIND)
#if KASPAN_VALGRIND != 0
#undef KASPAN_VALGRIND
#define KASPAN_VALGRIND 1
#endif
#else
#define KASPAN_VALGRIND 0
#endif
#if KASPAN_VALGRIND
#include <valgrind/callgrind.h>
#include <valgrind/memcheck.h>
#include <valgrind/valgrind.h>
#endif

// clang-format off

#define KASPAN_VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed) IF(KASPAN_VALGRIND, VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed))
#define KASPAN_VALGRIND_FREELIKE_BLOCK(_addr, _rzB)                       IF(KASPAN_VALGRIND, VALGRIND_FREELIKE_BLOCK(_addr, _rzB))

#define KASPAN_VALGRIND_MAKE_MEM_NOACCESS(_addr, _len)                    IF(KASPAN_VALGRIND, VALGRIND_MAKE_MEM_NOACCESS(_addr, _len))
#define KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len)                   IF(KASPAN_VALGRIND, VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len))
#define KASPAN_VALGRIND_MAKE_MEM_DEFINED(_addr, _len)                     IF(KASPAN_VALGRIND, VALGRIND_MAKE_MEM_DEFINED(_addr, _len))

#define KASPAN_VALGRIND_MAKE_VALUE_NOACCESS(_val)                         KASPAN_VALGRIND_MAKE_MEM_NOACCESS(&_val, sizeof(_val))
#define KASPAN_VALGRIND_MAKE_VALUE_UNDEFINED(_val)                        KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(&_val, sizeof(_val))
#define KASPAN_VALGRIND_MAKE_VALUE_DEFINED(_val)                          KASPAN_VALGRIND_MAKE_MEM_DEFINED(&_val, sizeof(_val))

#define KASPAN_VALGRIND_CHECK_MEM_IS_DEFINED(_addr, _len)                 IF(KASPAN_VALGRIND, VALGRIND_CHECK_MEM_IS_DEFINED(_addr, _len))
#define KASPAN_VALGRIND_CHECK_MEM_IS_ADDRESSABLE(_addr, _len)             IF(KASPAN_VALGRIND, VALGRIND_CHECK_MEM_IS_ADDRESSABLE(_addr, _len))
#define KASPAN_VALGRIND_CHECK_VALUE_IS_DEFINED(_val)                      IF(KASPAN_VALGRIND, VALGRIND_CHECK_VALUE_IS_DEFINED(_val))

#define KASPAN_VALGRIND_PRINTF(FORMAT, ...)                               IF(KASPAN_VALGRIND, VALGRIND_PRINTF(FORMAT __VA_OPT__(, __VA_ARGS__)))
#define KASPAN_VALGRIND_PRINTF_BACKTRACE(FORMAT, ...)                     IF(KASPAN_VALGRIND, VALGRIND_PRINTF_BACKTRACE(FORMAT __VA_OPT__(, __VA_ARGS__)))
#define KASPAN_VALGRIND_BACKTRACE()                                       IF(KASPAN_VALGRIND, VALGRIND_PRINTF_BACKTRACE("[BACKTRACE] user requested backtrace"))

#define KASPAN_CALLGRIND_START_INSTRUMENTATION()                          IF(KASPAN_VALGRIND, CALLGRIND_START_INSTRUMENTATION)
#define KASPAN_CALLGRIND_STOP_INSTRUMENTATION()                           IF(KASPAN_VALGRIND, CALLGRIND_STOP_INSTRUMENTATION)

// clang-format on
