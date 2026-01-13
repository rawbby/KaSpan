#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/memory/accessor/bits.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <iostream>

using namespace kaspan;

void
test_bits()
{
  bits b(100);

  b.clear(100);
  for (u64 i = 0; i < 100; ++i) {
    ASSERT_EQ(b.get(i), false);
  }

  b.set(5);
  ASSERT_EQ(b.get(5), true);
  b.unset(5);
  ASSERT_EQ(b.get(5), false);

  b.fill(100);
  for (u64 i = 0; i < 100; ++i) {
    ASSERT_EQ(b.get(i), true);
  }
}

void
test_bits_accessor()
{
  u64 data[2];
#if KASPAN_DEBUG
  bits_accessor ba(data, 128);
  ba.clear(128);
  ba.set(65);
  ASSERT_EQ(ba.get(65), true);
  ASSERT_EQ(ba.get(64), false);
#endif
}

int
main()
{
  std::cout << "Starting test_bits..." << std::endl;
  test_bits();
  test_bits_accessor();
  std::cout << "test_bits passed!" << std::endl;
  return 0;
}
