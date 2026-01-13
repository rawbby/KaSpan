#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/math.hpp>

#include <limits>
#include <random>

using namespace kaspan;

void
test_fuzzy()
{
  std::mt19937_64 rng(42);

  for (u64 base = 1; base <= 128; ++base) {
    for (int i = 0; i < 10000; ++i) {
      auto       dist = std::uniform_int_distribution<u64>{ 0, std::numeric_limits<u64>::max() - base + 1 };
      auto const x    = dist(rng);

      ASSERT_EQ(floordiv(x, base), x / base);
      ASSERT_EQ(ceildiv(x, base), (base - 1 + x) / base);
      ASSERT_EQ(remainder(x, base), x % base);
      ASSERT_EQ(round_down(x, base), x - (x % base));
      ASSERT_EQ(round_up(x, base), (x % base) ? x + (base - (x % base)) : x);
    }
  }
}

int
main()
{
  static_assert(floordiv<3>(0U) == 0);
  static_assert(floordiv<3>(1U) == 0);
  static_assert(floordiv<3>(2U) == 0);
  static_assert(floordiv<3>(3U) == 1);
  static_assert(floordiv<3>(4U) == 1);
  static_assert(floordiv<3>(5U) == 1);
  static_assert(floordiv<3>(6U) == 2);
  static_assert(floordiv<3>(7U) == 2);

  static_assert(floordiv<1>(0U) == 0);
  static_assert(floordiv<1>(1U) == 1);
  static_assert(floordiv<1>(2U) == 2);
  static_assert(floordiv<1>(3U) == 3);
  static_assert(floordiv<1>(4U) == 4);
  static_assert(floordiv<1>(5U) == 5);
  static_assert(floordiv<1>(6U) == 6);
  static_assert(floordiv<1>(7U) == 7);

  static_assert(floordiv<4>(0U) == 0);
  static_assert(floordiv<4>(1U) == 0);
  static_assert(floordiv<4>(2U) == 0);
  static_assert(floordiv<4>(3U) == 0);
  static_assert(floordiv<4>(4U) == 1);
  static_assert(floordiv<4>(5U) == 1);
  static_assert(floordiv<4>(6U) == 1);
  static_assert(floordiv<4>(7U) == 1);

  static_assert(ceildiv<3>(0U) == 0);
  static_assert(ceildiv<3>(1U) == 1);
  static_assert(ceildiv<3>(2U) == 1);
  static_assert(ceildiv<3>(3U) == 1);
  static_assert(ceildiv<3>(4U) == 2);
  static_assert(ceildiv<3>(5U) == 2);
  static_assert(ceildiv<3>(6U) == 2);
  static_assert(ceildiv<3>(7U) == 3);

  static_assert(ceildiv<1>(0U) == 0);
  static_assert(ceildiv<1>(1U) == 1);
  static_assert(ceildiv<1>(2U) == 2);
  static_assert(ceildiv<1>(3U) == 3);
  static_assert(ceildiv<1>(4U) == 4);
  static_assert(ceildiv<1>(5U) == 5);
  static_assert(ceildiv<1>(6U) == 6);
  static_assert(ceildiv<1>(7U) == 7);

  static_assert(ceildiv<4>(0U) == 0);
  static_assert(ceildiv<4>(1U) == 1);
  static_assert(ceildiv<4>(2U) == 1);
  static_assert(ceildiv<4>(3U) == 1);
  static_assert(ceildiv<4>(4U) == 1);
  static_assert(ceildiv<4>(5U) == 2);
  static_assert(ceildiv<4>(6U) == 2);
  static_assert(ceildiv<4>(7U) == 2);

  static_assert(remainder<3>(0U) == 0);
  static_assert(remainder<3>(1U) == 1);
  static_assert(remainder<3>(2U) == 2);
  static_assert(remainder<3>(3U) == 0);
  static_assert(remainder<3>(4U) == 1);
  static_assert(remainder<3>(5U) == 2);
  static_assert(remainder<3>(6U) == 0);
  static_assert(remainder<3>(7U) == 1);

  static_assert(remainder<1>(0U) == 0);
  static_assert(remainder<1>(1U) == 0);
  static_assert(remainder<1>(2U) == 0);
  static_assert(remainder<1>(3U) == 0);
  static_assert(remainder<1>(4U) == 0);
  static_assert(remainder<1>(5U) == 0);
  static_assert(remainder<1>(6U) == 0);
  static_assert(remainder<1>(7U) == 0);

  static_assert(remainder<4>(0U) == 0);
  static_assert(remainder<4>(1U) == 1);
  static_assert(remainder<4>(2U) == 2);
  static_assert(remainder<4>(3U) == 3);
  static_assert(remainder<4>(4U) == 0);
  static_assert(remainder<4>(5U) == 1);
  static_assert(remainder<4>(6U) == 2);
  static_assert(remainder<4>(7U) == 3);

  static_assert(round_down<3>(0U) == 0);
  static_assert(round_down<3>(1U) == 0);
  static_assert(round_down<3>(2U) == 0);
  static_assert(round_down<3>(3U) == 3);
  static_assert(round_down<3>(4U) == 3);
  static_assert(round_down<3>(5U) == 3);
  static_assert(round_down<3>(6U) == 6);
  static_assert(round_down<3>(7U) == 6);

  static_assert(round_down<1>(0U) == 0);
  static_assert(round_down<1>(1U) == 1);
  static_assert(round_down<1>(2U) == 2);
  static_assert(round_down<1>(3U) == 3);
  static_assert(round_down<1>(4U) == 4);
  static_assert(round_down<1>(5U) == 5);
  static_assert(round_down<1>(6U) == 6);
  static_assert(round_down<1>(7U) == 7);

  static_assert(round_down<4>(0U) == 0);
  static_assert(round_down<4>(1U) == 0);
  static_assert(round_down<4>(2U) == 0);
  static_assert(round_down<4>(3U) == 0);
  static_assert(round_down<4>(4U) == 4);
  static_assert(round_down<4>(5U) == 4);
  static_assert(round_down<4>(6U) == 4);
  static_assert(round_down<4>(7U) == 4);

  static_assert(round_up<3>(0U) == 0);
  static_assert(round_up<3>(1U) == 3);
  static_assert(round_up<3>(2U) == 3);
  static_assert(round_up<3>(3U) == 3);
  static_assert(round_up<3>(4U) == 6);
  static_assert(round_up<3>(5U) == 6);
  static_assert(round_up<3>(6U) == 6);
  static_assert(round_up<3>(7U) == 9);

  static_assert(round_up<1>(0U) == 0);
  static_assert(round_up<1>(1U) == 1);
  static_assert(round_up<1>(2U) == 2);
  static_assert(round_up<1>(3U) == 3);
  static_assert(round_up<1>(4U) == 4);
  static_assert(round_up<1>(5U) == 5);
  static_assert(round_up<1>(6U) == 6);
  static_assert(round_up<1>(7U) == 7);

  static_assert(round_up<4>(0U) == 0);
  static_assert(round_up<4>(1U) == 4);
  static_assert(round_up<4>(2U) == 4);
  static_assert(round_up<4>(3U) == 4);
  static_assert(round_up<4>(4U) == 4);
  static_assert(round_up<4>(5U) == 8);
  static_assert(round_up<4>(6U) == 8);
  static_assert(round_up<4>(7U) == 8);

  static_assert(clip<0, 5>(0U) == 0);
  static_assert(clip<0, 5>(1U) == 1);
  static_assert(clip<0, 5>(2U) == 2);
  static_assert(clip<0, 5>(3U) == 3);
  static_assert(clip<0, 5>(4U) == 4);
  static_assert(clip<0, 5>(5U) == 5);
  static_assert(clip<0, 5>(6U) == 5);
  static_assert(clip<0, 5>(7U) == 5);

  static_assert(clip<2, 4>(0U) == 2);
  static_assert(clip<2, 4>(1U) == 2);
  static_assert(clip<2, 4>(2U) == 2);
  static_assert(clip<2, 4>(3U) == 3);
  static_assert(clip<2, 4>(4U) == 4);
  static_assert(clip<2, 4>(5U) == 4);
  static_assert(clip<2, 4>(6U) == 4);
  static_assert(clip<2, 4>(7U) == 4);

  test_fuzzy();
}
