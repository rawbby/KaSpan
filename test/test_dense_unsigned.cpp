#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/dense_unsigned_accessor.hpp>
#include <kaspan/memory/accessor/dense_unsigned_array.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <cstddef>
#include <iostream>

using namespace kaspan;

void
test_dense_unsigned()
{
  dense_unsigned_array<u64> dua(10, 3); // 10 elements, 3 bytes per element
  dua.fill(0, 10);
  dua.set(5, 0x123456);
  ASSERT_EQ(dua.get(5), 0x123456);
  ASSERT_EQ(dua.get(4), 0);
}

void
test_dense_unsigned_accessor()
{
  std::byte                    data[30];
  dense_unsigned_accessor<u64> dua(data, 10, 3);
  dua.set(0, 0xABCDEF);
  ASSERT_EQ(dua.get(0), 0xABCDEF);
}

int
main()
{
  std::cout << "Starting test_dense_unsigned..." << std::endl;
  test_dense_unsigned();
  test_dense_unsigned_accessor();
  std::cout << "test_dense_unsigned passed!" << std::endl;
  return 0;
}
