#include <iostream>
#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <random>
#include <vector>

using namespace kaspan;

void
test_basic()
{
  u64 data[10] = {};
#if KASPAN_DEBUG
  bits_accessor bits(data, 10 * 64);

  bits.set(5);
  ASSERT_EQ(bits.get(5), true);
  ASSERT_EQ(bits.get(4), false);

  bits.unset(5);
  ASSERT_EQ(bits.get(5), false);

  bits.set(65);
  ASSERT_EQ(bits.get(65), true);

  bits.clear(100);
  ASSERT_EQ(bits.get(65), false);

  bits.fill(70);
  ASSERT_EQ(bits.get(0), true);
  ASSERT_EQ(bits.get(63), true);
  ASSERT_EQ(bits.get(69), true);
#endif
}

void
test_random()
{
  std::vector<u64> storage(100, 0);
  u64              n = 100 * 64;
#if KASPAN_DEBUG
  bits_accessor bits(storage.data(), n);

  std::mt19937_64                    rng(42);
  std::uniform_int_distribution<u64> dist(0, n - 1);

  std::vector<bool> expected(n, false);
  for (int i = 0; i < 1000; ++i) {
    u64  idx = dist(rng);
    bool val = (i % 2 == 0);
    bits.set(idx, val);
    expected[idx] = val;
  }

  for (u64 i = 0; i < n; ++i) {
    ASSERT_EQ(bits.get(i), expected[i]);
  }
#endif
}

int
main()
{
  std::cout << "Starting test_bits_accessor..." << std::endl;
  test_basic();
  test_random();
  std::cout << "test_bits_accessor passed!" << std::endl;
  return 0;
}
