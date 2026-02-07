#include <kaspan/memory/dense_unsigned_ops.hpp>
#include <kaspan/debug/assert.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <bit>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <random>
#include <vector>

using namespace kaspan;

void
test_basic()
{
  std::byte   data[100]     = {};
  u8          element_bytes = 3;
  std::endian endian        = std::endian::little;
  u64         val           = 0x123456;

  dense_unsigned_ops::set<u64>(data, 0, element_bytes, endian, val);
  u64 got = dense_unsigned_ops::get<u64>(data, 0, element_bytes, endian);
  ASSERT_EQ(got, val);

  dense_unsigned_ops::fill<u64>(data, 10, element_bytes, endian, 0xABCDEF);
  for (u64 i = 0; i < 10; ++i) {
    ASSERT_EQ(dense_unsigned_ops::get<u64>(data, i, element_bytes, endian), 0xABCDEF);
  }
}

void
test_random()
{
  std::byte       data[1024] = {};
  std::mt19937_64 rng(42);

  for (u8 element_bytes = 1; element_bytes <= 8; ++element_bytes) {
    for (auto endian : { std::endian::little, std::endian::big }) {
      u64                                max_val = (element_bytes == 8) ? std::numeric_limits<u64>::max() : (1ULL << (element_bytes * 8)) - 1;
      std::uniform_int_distribution<u64> dist(0, max_val);

      std::vector<u64> expected(10);
      for (int i = 0; i < 10; ++i) {
        expected[i] = dist(rng);
        dense_unsigned_ops::set<u64>(data, i, element_bytes, endian, expected[i]);
      }

      for (int i = 0; i < 10; ++i) {
        u64 got = dense_unsigned_ops::get<u64>(data, i, element_bytes, endian);
        ASSERT_EQ(got, expected[i]);
      }
    }
  }
}

int
main()
{
  test_basic();
  test_random();
  return 0;
}
