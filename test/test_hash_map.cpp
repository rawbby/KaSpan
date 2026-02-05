#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/hash_map.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>

void
test()
{
  using namespace kaspan;

  auto       rng = std::mt19937{ std::random_device{}() };
  auto const n   = std::uniform_int_distribution<u32>{}(rng);
  auto const beg = std::uniform_int_distribution{ 0u, n }(rng);
  auto const end = std::uniform_int_distribution{ beg, n }(rng);

  auto non_local_lo = std::uniform_int_distribution{ 0u, beg };
  auto non_local_hi = std::uniform_int_distribution{ end, n };

  hash_map<u32>                map{ 5000 };
  std::unordered_map<u32, u32> inserted{};

  auto last_map_count = map.count();

  u32 count = 0;
  for (int i = 0; i < 2500; ++i) {
    auto const key = non_local_lo(rng);
    auto const val = map.get_or_insert(key, [&]{ return count++; });
    ASSERT_EQ(val, map.get(key));

    if (last_map_count == map.count()) {
      ASSERT(inserted.contains(key));
      ASSERT_EQ(inserted[key], val);
    } else {
      ASSERT_NOT(inserted.contains(key));
      inserted[key] = val;
    }
    last_map_count = map.count();
  }

  for (int i = 0; i < 2500; ++i) {
    auto const key = non_local_hi(rng);
    auto const val = map.get_or_insert(key, [&]{ return count++; });
    ASSERT_EQ(val, map.get(key));

    if (last_map_count == map.count()) {
      ASSERT(inserted.contains(key));
      ASSERT_EQ(inserted[key], val);
    } else {
      ASSERT_NOT(inserted.contains(key));
      inserted[key] = val;
    }
    last_map_count = map.count();
  }

  ASSERT_EQ(map.count(), inserted.size());
  for (auto const [key, val] : inserted) {
    ASSERT_EQ(val, map.get(key));
  }
}

int
main()
{
  for (int i = 0; i < 5000; ++i) {
    test();
  }
}
