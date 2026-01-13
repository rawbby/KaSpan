#include <kaspan/debug/assert_false.hpp>
#include <kaspan/debug/assert_ge.hpp>
#include <kaspan/debug/assert_gt.hpp>
#include <kaspan/debug/assert_le.hpp>
#include <kaspan/debug/assert_lt.hpp>
#include <kaspan/debug/assert_ne.hpp>
#include <kaspan/debug/assert_true.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstdint>
#include <limits>

using namespace kaspan;

int
main()
{
  // --- Corner Case 1: Signed -1 vs Unsigned 1 ---
  {
    // Standard comparison: -1 is converted to a large unsigned value.
    // So -1 < 1u evaluates to (4294967295u < 1u) which is false.
    constexpr auto s_minus_1 = -1;
    constexpr auto u_1       = 1u;

    // assert the "wrong" from an arithmetic standpoint
    ASSERT_NOT(s_minus_1 < u_1);
    ASSERT(s_minus_1 > u_1);

    // assert the "right" from an arithmetic standpoint
    ASSERT_LT(s_minus_1, u_1);
    ASSERT_GE(u_1, s_minus_1);
  }

  // --- Corner Case 2: Signed -1 vs Maximum Unsigned ---
  {
    // Standard comparison: -1 == UINT_MAX
    constexpr auto s_minus_1 = -1;
    constexpr auto u_max     = std::numeric_limits<u32>::max();

    // Assert standard comparison "wrong" result (it says they are equal)
    ASSERT(s_minus_1 == u_max);

    // Assert our macro "right" result (they are NOT equal)
    ASSERT_NE(s_minus_1, u_max);
    ASSERT_LT(s_minus_1, u_max);
  }

  // --- Corner Case 3: Large Signed vs Large Unsigned ---
  {
    // If we compare a large positive signed value with an unsigned value of the same bit-width,
    // it usually works fine unless one is negative.
    // But let's check INT_MIN vs 0u.
    constexpr auto s_min = std::numeric_limits<i32>::min();
    constexpr auto u_0   = 0u;

    // Standard comparison: s_min < 0u is FALSE because s_min is converted to a large unsigned.
    ASSERT_NOT(s_min < u_0);
    ASSERT(s_min > u_0);

    // Our macros
    ASSERT_LT(s_min, u_0);
    ASSERT_LE(s_min, u_0);
    ASSERT_GT(u_0, s_min);
  }

  // --- Corner Case 4: 16-bit vs 32-bit (Promotion) ---
  {
    // Comparison between different widths can also be tricky if signs differ.
    int16_t const  s16_minus_1 = -1;
    uint32_t const u32_1       = 1;

    // Standard: s16_minus_1 is promoted to int32_t (-1), then to uint32_t (4294967295u).
    ASSERT_NOT(s16_minus_1 < u32_1);

    // Our macros
    ASSERT_LT(s16_minus_1, u32_1);
  }
}
