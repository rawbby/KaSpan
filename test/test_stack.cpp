#include <kaspan/debug/assert.hpp>
#include <kaspan/memory/accessor/stack.hpp>
#include <kaspan/memory/accessor/stack_accessor.hpp>

#include <iostream>

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
  std::cout << "Starting test_stack..." << std::endl;
  test_stack();
  test_stack_accessor();
  std::cout << "test_stack passed!" << std::endl;
  return 0;
}
