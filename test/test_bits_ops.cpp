#include <iostream>
#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/memory/accessor/bits_ops.hpp>
#include <random>
#include <vector>

using namespace kaspan;

void
test_ops_basic()
{
  u64 data[10] = {};

  bits_ops::set(data, 5);
  ASSERT_EQ(bits_ops::get(data, 5), true);
  ASSERT_EQ(bits_ops::get(data, 4), false);

  bits_ops::unset(data, 5);
  ASSERT_EQ(bits_ops::get(data, 5), false);

  bits_ops::set(data, 65);
  ASSERT_EQ(bits_ops::get(data, 65), true);

  bits_ops::clear(data, 100);
  ASSERT_EQ(bits_ops::get(data, 65), false);

  bits_ops::fill(data, 70);
  ASSERT_EQ(bits_ops::get(data, 0), true);
  ASSERT_EQ(bits_ops::get(data, 63), true);
  ASSERT_EQ(bits_ops::get(data, 69), true);
}

void
test_ops_for_each()
{
  u64 data[2] = { 0, 0 };
  bits_ops::set(data, 5);
  bits_ops::set(data, 65);
  bits_ops::set(data, 127);

  std::vector<u64> found;
  bits_ops::for_each(data, 128, [&](u64 idx) {
    found.push_back(idx);
  });

  ASSERT_EQ(found.size(), 3);
  ASSERT_EQ(found[0], 5);
  ASSERT_EQ(found[1], 65);
  ASSERT_EQ(found[2], 127);
}

int
main()
{
  std::cout << "Starting test_bits_ops..." << std::endl;
  test_ops_basic();
  test_ops_for_each();
  std::cout << "test_bits_ops passed!" << std::endl;
  return 0;
}
