#include <kaspan/memory/hash_set.hpp>
#include <kaspan/debug/assert.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_set>

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

  hash_set<u32>           map{ 3000 };
  std::unordered_set<u32> inserted{};

  for (int i = 0; i < 1500; ++i) {
    auto const key = non_local_lo(rng);
    map.insert(key);
    inserted.insert(key);
    ASSERT(map.contains(key));
    ASSERT(inserted.contains(key));
  }

  for (int i = 0; i < 1500; ++i) {
    auto const key = non_local_hi(rng);
    map.insert(key);
    inserted.insert(key);
    ASSERT(map.contains(key));
    ASSERT(inserted.contains(key));
  }

  ASSERT_EQ(map.count(), inserted.size());
  for (auto const key : inserted) {
    ASSERT(map.contains(key));
  }
}

int
main()
{
  for (int i = 0; i < 1000; ++i) {
    test();
  }
}
