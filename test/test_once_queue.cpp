#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/memory/accessor/once_queue.hpp>
#include <kaspan/memory/accessor/once_queue_accessor.hpp>

using namespace kaspan;

int
main()
{
  auto queue = make_once_queue<int>(10);
  ASSERT_EQ(queue.size(), 0);
  ASSERT_EQ(queue.empty(), true);

  queue.push_back(1);
  ASSERT_EQ(queue.size(), 1);
  ASSERT_EQ(queue.data()[0], 1);

  queue.push_back(2);
  ASSERT_EQ(queue.size(), 2);
  ASSERT_EQ(queue.data()[1], 2);

  queue.push_back(3);
  ASSERT_EQ(queue.size(), 3);
  ASSERT_EQ(queue.data()[2], 3);

  int                      data[10];
  once_queue_accessor<int> oqa(data, 10);
  oqa.push_back(42);
  ASSERT_EQ(oqa.size(), 1);
  ASSERT_EQ(oqa.data()[0], 42);

  queue.clear();
  ASSERT_EQ(queue.size(), 0);
  ASSERT_EQ(queue.empty(), true);

  return 0;
}
