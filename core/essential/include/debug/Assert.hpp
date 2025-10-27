#pragma once

#include <debug/Debug.hpp>
#include <pp/PP.hpp>
#include <test/Assert.hpp>

#define DEBUG_ASSERT(...) IF(KASPAN_DEBUG, ASSERT(__VA_ARGS__))
#define DEBUG_ASSERT_LT(...) IF(KASPAN_DEBUG, ASSERT_LT(__VA_ARGS__))
#define DEBUG_ASSERT_GT(...) IF(KASPAN_DEBUG, ASSERT_GT(__VA_ARGS__))
#define DEBUG_ASSERT_LE(...) IF(KASPAN_DEBUG, ASSERT_LE(__VA_ARGS__))
#define DEBUG_ASSERT_GE(...) IF(KASPAN_DEBUG, ASSERT_GE(__VA_ARGS__))
#define DEBUG_ASSERT_EQ(...) IF(KASPAN_DEBUG, ASSERT_EQ(__VA_ARGS__))
#define DEBUG_ASSERT_NE(...) IF(KASPAN_DEBUG, ASSERT_NE(__VA_ARGS__))
