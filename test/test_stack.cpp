#include <kaspan/memory/stack.hpp>
#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/stack_accessor.hpp>

using namespace kaspan;

void
test_stack()
{
  stack<int> s(10);
  ASSERT_EQ(s.size(), 0);
  s.push(1);
  s.push(2);
  ASSERT_EQ(s.size(), 2);
  ASSERT_EQ(s.data()[0], 1);
  ASSERT_EQ(s.data()[1], 2);
  ASSERT_EQ(s.back(), 2);
  s.pop();
  ASSERT_EQ(s.size(), 1);
  ASSERT_EQ(s.back(), 1);
}

void
test_stack_accessor()
{
  int                 data[10];
  stack_accessor<int> sa(data, 10);
  sa.push(10);
  ASSERT_EQ(sa.size(), 1);
  ASSERT_EQ(sa.data()[0], 10);
}

int
main()
{
  test_stack();
  test_stack_accessor();
  return 0;
}
