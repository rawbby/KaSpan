#include <util/math.hpp>

#include <random>

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
  static_assert(floordiv<3>(0u) == 0);
  static_assert(floordiv<3>(1u) == 0);
  static_assert(floordiv<3>(2u) == 0);
  static_assert(floordiv<3>(3u) == 1);
  static_assert(floordiv<3>(4u) == 1);
  static_assert(floordiv<3>(5u) == 1);
  static_assert(floordiv<3>(6u) == 2);
  static_assert(floordiv<3>(7u) == 2);

  static_assert(floordiv<1>(0u) == 0);
  static_assert(floordiv<1>(1u) == 1);
  static_assert(floordiv<1>(2u) == 2);
  static_assert(floordiv<1>(3u) == 3);
  static_assert(floordiv<1>(4u) == 4);
  static_assert(floordiv<1>(5u) == 5);
  static_assert(floordiv<1>(6u) == 6);
  static_assert(floordiv<1>(7u) == 7);

  static_assert(floordiv<4>(0u) == 0);
  static_assert(floordiv<4>(1u) == 0);
  static_assert(floordiv<4>(2u) == 0);
  static_assert(floordiv<4>(3u) == 0);
  static_assert(floordiv<4>(4u) == 1);
  static_assert(floordiv<4>(5u) == 1);
  static_assert(floordiv<4>(6u) == 1);
  static_assert(floordiv<4>(7u) == 1);

  static_assert(ceildiv<3>(0u) == 0);
  static_assert(ceildiv<3>(1u) == 1);
  static_assert(ceildiv<3>(2u) == 1);
  static_assert(ceildiv<3>(3u) == 1);
  static_assert(ceildiv<3>(4u) == 2);
  static_assert(ceildiv<3>(5u) == 2);
  static_assert(ceildiv<3>(6u) == 2);
  static_assert(ceildiv<3>(7u) == 3);

  static_assert(ceildiv<1>(0u) == 0);
  static_assert(ceildiv<1>(1u) == 1);
  static_assert(ceildiv<1>(2u) == 2);
  static_assert(ceildiv<1>(3u) == 3);
  static_assert(ceildiv<1>(4u) == 4);
  static_assert(ceildiv<1>(5u) == 5);
  static_assert(ceildiv<1>(6u) == 6);
  static_assert(ceildiv<1>(7u) == 7);

  static_assert(ceildiv<4>(0u) == 0);
  static_assert(ceildiv<4>(1u) == 1);
  static_assert(ceildiv<4>(2u) == 1);
  static_assert(ceildiv<4>(3u) == 1);
  static_assert(ceildiv<4>(4u) == 1);
  static_assert(ceildiv<4>(5u) == 2);
  static_assert(ceildiv<4>(6u) == 2);
  static_assert(ceildiv<4>(7u) == 2);

  static_assert(remainder<3>(0u) == 0);
  static_assert(remainder<3>(1u) == 1);
  static_assert(remainder<3>(2u) == 2);
  static_assert(remainder<3>(3u) == 0);
  static_assert(remainder<3>(4u) == 1);
  static_assert(remainder<3>(5u) == 2);
  static_assert(remainder<3>(6u) == 0);
  static_assert(remainder<3>(7u) == 1);

  static_assert(remainder<1>(0u) == 0);
  static_assert(remainder<1>(1u) == 0);
  static_assert(remainder<1>(2u) == 0);
  static_assert(remainder<1>(3u) == 0);
  static_assert(remainder<1>(4u) == 0);
  static_assert(remainder<1>(5u) == 0);
  static_assert(remainder<1>(6u) == 0);
  static_assert(remainder<1>(7u) == 0);

  static_assert(remainder<4>(0u) == 0);
  static_assert(remainder<4>(1u) == 1);
  static_assert(remainder<4>(2u) == 2);
  static_assert(remainder<4>(3u) == 3);
  static_assert(remainder<4>(4u) == 0);
  static_assert(remainder<4>(5u) == 1);
  static_assert(remainder<4>(6u) == 2);
  static_assert(remainder<4>(7u) == 3);

  static_assert(round_down<3>(0u) == 0);
  static_assert(round_down<3>(1u) == 0);
  static_assert(round_down<3>(2u) == 0);
  static_assert(round_down<3>(3u) == 3);
  static_assert(round_down<3>(4u) == 3);
  static_assert(round_down<3>(5u) == 3);
  static_assert(round_down<3>(6u) == 6);
  static_assert(round_down<3>(7u) == 6);

  static_assert(round_down<1>(0u) == 0);
  static_assert(round_down<1>(1u) == 1);
  static_assert(round_down<1>(2u) == 2);
  static_assert(round_down<1>(3u) == 3);
  static_assert(round_down<1>(4u) == 4);
  static_assert(round_down<1>(5u) == 5);
  static_assert(round_down<1>(6u) == 6);
  static_assert(round_down<1>(7u) == 7);

  static_assert(round_down<4>(0u) == 0);
  static_assert(round_down<4>(1u) == 0);
  static_assert(round_down<4>(2u) == 0);
  static_assert(round_down<4>(3u) == 0);
  static_assert(round_down<4>(4u) == 4);
  static_assert(round_down<4>(5u) == 4);
  static_assert(round_down<4>(6u) == 4);
  static_assert(round_down<4>(7u) == 4);

  static_assert(round_up<3>(0u) == 0);
  static_assert(round_up<3>(1u) == 3);
  static_assert(round_up<3>(2u) == 3);
  static_assert(round_up<3>(3u) == 3);
  static_assert(round_up<3>(4u) == 6);
  static_assert(round_up<3>(5u) == 6);
  static_assert(round_up<3>(6u) == 6);
  static_assert(round_up<3>(7u) == 9);

  static_assert(round_up<1>(0u) == 0);
  static_assert(round_up<1>(1u) == 1);
  static_assert(round_up<1>(2u) == 2);
  static_assert(round_up<1>(3u) == 3);
  static_assert(round_up<1>(4u) == 4);
  static_assert(round_up<1>(5u) == 5);
  static_assert(round_up<1>(6u) == 6);
  static_assert(round_up<1>(7u) == 7);

  static_assert(round_up<4>(0u) == 0);
  static_assert(round_up<4>(1u) == 4);
  static_assert(round_up<4>(2u) == 4);
  static_assert(round_up<4>(3u) == 4);
  static_assert(round_up<4>(4u) == 4);
  static_assert(round_up<4>(5u) == 8);
  static_assert(round_up<4>(6u) == 8);
  static_assert(round_up<4>(7u) == 8);

  static_assert(clip<0, 5>(0u) == 0);
  static_assert(clip<0, 5>(1u) == 1);
  static_assert(clip<0, 5>(2u) == 2);
  static_assert(clip<0, 5>(3u) == 3);
  static_assert(clip<0, 5>(4u) == 4);
  static_assert(clip<0, 5>(5u) == 5);
  static_assert(clip<0, 5>(6u) == 5);
  static_assert(clip<0, 5>(7u) == 5);

  static_assert(clip<2, 4>(0u) == 2);
  static_assert(clip<2, 4>(1u) == 2);
  static_assert(clip<2, 4>(2u) == 2);
  static_assert(clip<2, 4>(3u) == 3);
  static_assert(clip<2, 4>(4u) == 4);
  static_assert(clip<2, 4>(5u) == 4);
  static_assert(clip<2, 4>(6u) == 4);
  static_assert(clip<2, 4>(7u) == 4);

  test_fuzzy();
}
