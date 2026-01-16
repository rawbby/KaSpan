#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>

using namespace kaspan;

constexpr int
test_assertions()
{
  ASSERT(true);
  ASSERT(true, "details");
  ASSERT(true, "details {}", 1);

  ASSERT_NE(1, 2);
  ASSERT_NE(1, 2, "details");
  ASSERT_NE(1, 2, "details {}", 1);

  ASSERT_LT(1, 2);
  ASSERT_LT(1, 2, "details");
  ASSERT_LT(1, 2, "details {}", 1);

  ASSERT_LE(1, 1);
  ASSERT_LE(1, 2);
  ASSERT_LE(1, 2, "details");
  ASSERT_LE(1, 2, "details {}", 1);

  ASSERT_GT(2, 1);
  ASSERT_GT(2, 1, "details");
  ASSERT_GT(2, 1, "details {}", 1);

  ASSERT_GE(2, 2);
  ASSERT_GE(2, 1);
  ASSERT_GE(2, 1, "details");
  ASSERT_GE(2, 1, "details {}", 1);

  ASSERT_NOT(false);
  ASSERT_NOT(false, "details");
  ASSERT_NOT(false, "details {}", 1);

  ASSERT_EQ(1, 1);
  ASSERT_EQ(1, 1, "details");
  ASSERT_EQ(1, 1, "details {}", 1);

  ASSERT_IN_RANGE(5, 0, 10);
  ASSERT_IN_RANGE(5, 0, 10, "details");
  ASSERT_IN_RANGE(5, 0, 10, "details {}", 1);

  ASSERT_IN_RANGE_INCLUSIVE(5, 0, 5);
  ASSERT_IN_RANGE_INCLUSIVE(0, 0, 5);
  ASSERT_IN_RANGE_INCLUSIVE(5, 0, 5, "details");
  ASSERT_IN_RANGE_INCLUSIVE(5, 0, 5, "details {}", 1);

  return EXIT_SUCCESS;
}

constexpr int
test_assert_cmp()
{
  return test_assertions();

  constexpr auto i32_minus_1 = -1;
  constexpr auto u32_1       = 1u;
  constexpr auto u32_max     = std::numeric_limits<u32>::max();

  // === Corner Case 1: signed < unsigned ===
  // Standard: signed is converted unsigned:
  // -1 < 1u evaluates as (4294967295u < 1u) which is false.
  // Assert: should be arithmetically correct
  ASSERT_NE(i32_minus_1, u32_1);
  ASSERT_LT(i32_minus_1, u32_1);
  ASSERT_LE(i32_minus_1, u32_1);
  ASSERT_GT(u32_1, i32_minus_1);
  ASSERT_GE(u32_1, i32_minus_1);

  // === Corner Case 1: signed == unsigned ===
  // Standard: signed is converted unsigned:
  // -1 == UINT_MAX evaluates as (4294967295u == 4294967295u) which is true.
  // Assert our macro "right" result (they are NOT equal)
  ASSERT_NE(i32_minus_1, u32_max);
  ASSERT_LT(i32_minus_1, u32_max);
  ASSERT_LE(i32_minus_1, u32_max);
  ASSERT_GT(u32_max, i32_minus_1);
  ASSERT_GE(u32_max, i32_minus_1);

  return EXIT_SUCCESS;
}

static_assert(test_assertions() == EXIT_SUCCESS);
static_assert(test_assert_cmp() == EXIT_SUCCESS);

int
main()
{
  std::ignore = test_assertions();
  std::ignore = test_assert_cmp();
}
