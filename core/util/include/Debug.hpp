#pragma once

#include <ErrorCode>

#ifndef NDEBUG
#define DEBUG_ASSERT_TRY(...) ASSERT_TRY(__VA_ARGS__)
#else
#define DEBUG_ASSERT_TRY(...) \
do {                        \
} while (false)
#endif
