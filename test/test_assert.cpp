#include <kaspan/debug/assert.hpp>

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

static_assert(test_assertions() == EXIT_SUCCESS);

int
main()
{
  return test_assertions();
}
