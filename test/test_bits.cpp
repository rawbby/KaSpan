#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/bits.hpp>
#include <kaspan/memory/accessor/bits_accessor.hpp>
#include <kaspan/util/arithmetic.hpp>

#include <random>

using namespace kaspan;

void
test_bit_ops_each()
{
  u64 storage[2]{};
  bits_ops::set(storage, 5);
  bits_ops::set(storage, 65);
  bits_ops::set(storage, 127);

  std::vector<u64> found;
  bits_ops::each(storage, 128, [&](u64 idx) {
    found.push_back(idx);
  });

  ASSERT_EQ(found.size(), 3);
  ASSERT_EQ(found[0], 5);
  ASSERT_EQ(found[1], 65);
  ASSERT_EQ(found[2], 127);
}

void
test_bit_ops_basic()
{
  u64 storage[10]{};

  bits_ops::set(storage, 5);
  bits_ops::set(storage, 7);
  ASSERT_NOT(bits_ops::get(storage, 4));
  ASSERT(bits_ops::get(storage, 5));
  ASSERT_NOT(bits_ops::get(storage, 6));
  ASSERT(bits_ops::get(storage, 7));

  bits_ops::unset(storage, 5);
  ASSERT_NOT(bits_ops::get(storage, 4));
  ASSERT_NOT(bits_ops::get(storage, 5));
  ASSERT_NOT(bits_ops::get(storage, 6));
  ASSERT(bits_ops::get(storage, 7));

  bits_ops::set(storage, 65);
  ASSERT_NOT(bits_ops::get(storage, 63));
  ASSERT_NOT(bits_ops::get(storage, 64));
  ASSERT(bits_ops::get(storage, 65));
  ASSERT_NOT(bits_ops::get(storage, 66));

  bits_ops::clear(storage, 100);
  ASSERT_NOT(bits_ops::get(storage, 63));
  ASSERT_NOT(bits_ops::get(storage, 64));
  ASSERT_NOT(bits_ops::get(storage, 65));
  ASSERT_NOT(bits_ops::get(storage, 66));

  bits_ops::fill(storage, 70);
  ASSERT(bits_ops::get(storage, 0));
  ASSERT(bits_ops::get(storage, 63));
  ASSERT(bits_ops::get(storage, 64));
  ASSERT(bits_ops::get(storage, 65));
  ASSERT(bits_ops::get(storage, 66));
  ASSERT(bits_ops::get(storage, 69));
}

void
test_bits_random(
  auto& b)
{
  constexpr auto n = static_cast<u64>(128 * 64);

  auto rng      = std::mt19937_64{ std::random_device{}() };
  auto idx_dist = std::uniform_int_distribution<u64>{ 0, n - 1 };
  auto val_dist = std::uniform_int_distribution{ 0, 1 };

  // initialise reference and bits with zero
  auto expected = std::vector(n, false);
  b.clear(n);

  for (u64 i = 0; i < n; ++i) {
    auto const idx = idx_dist(rng);
    auto const val = static_cast<bool>(val_dist(rng));

    expected[idx] = val;
    b.set(idx, val);
  }

  for (u64 i = 0; i < n; ++i) {
    ASSERT_EQ(b.get(i), expected[i]);
  }
}

void
test_bits_basic(
  auto& b)
{
  b.fill(200);
  b.clear(100);

  for (u64 i = 0; i < 100; ++i) {
    ASSERT_NOT(b.get(i));
  }
  for (u64 i = 100; i < 200; ++i) {
    ASSERT(b.get(i));
  }

  auto it = 100;
  b.for_each(200, [&](auto i) {
    ASSERT_EQ(i, it++);
  });

  b.unset(4);
  b.set(5);
  b.unset(6);
  b.set(7);
  ASSERT_NOT(b.get(4));
  ASSERT(b.get(5));
  ASSERT_NOT(b.get(6));
  ASSERT(b.get(7));

  b.set(4);
  b.unset(5);
  b.set(6);
  b.unset(7);
  ASSERT(b.get(4));
  ASSERT_NOT(b.get(5));
  ASSERT(b.get(6));
  ASSERT_NOT(b.get(7));

  b.fill(100);
  for (u64 i = 0; i < 200; ++i) {
    ASSERT(b.get(i));
  }
}

int
main()
{
  test_bit_ops_basic();
  test_bit_ops_each();

  auto b = bits{ 128 * 64 };
  test_bits_basic(b);
  test_bits_random(b);

  u64  ba_storage[128];
  auto ba = bits_accessor{ ba_storage, 128 * 64 };
  test_bits_basic(ba);
  test_bits_random(b);
}
