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
#include <valgrind/memcheck.h>
#include <valgrind/valgrind.h>
#endif
#define KASPAN_VALGRIND_MAKE_MEM_NOACCESS(_addr, _len) IF(KASPAN_VALGRIND, VALGRIND_MAKE_MEM_NOACCESS(_addr, _len), ((void)0))
#define KASPAN_VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len) IF(KASPAN_VALGRIND, VALGRIND_MAKE_MEM_UNDEFINED(_addr, _len), ((void)0))
#define KASPAN_VALGRIND_MAKE_MEM_DEFINED(_addr, _len) IF(KASPAN_VALGRIND, VALGRIND_MAKE_MEM_DEFINED(_addr, _len), ((void)0))
#define KASPAN_VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed) IF(KASPAN_VALGRIND, VALGRIND_MALLOCLIKE_BLOCK(_addr, _sizeB, _rzB, _is_zeroed), ((void)0))
#define KASPAN_VALGRIND_FREELIKE_BLOCK(_addr, _rzB) IF(KASPAN_VALGRIND, VALGRIND_FREELIKE_BLOCK(_addr, _rzB), ((void)0))
#define KASPAN_VALGRIND_PRINTF(...) IF(KASPAN_VALGRIND, VALGRIND_PRINTF(__VA_ARGS__), ((void)0))
#define KASPAN_VALGRIND_PRINTF_BACKTRACE(...) IF(KASPAN_VALGRIND, VALGRIND_PRINTF_BACKTRACE(__VA_ARGS__), ((void)0))
