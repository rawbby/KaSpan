#include <pp/Describe.hpp>
#include <pp/PP.hpp>
#include <test/Assert.hpp>

#include <string_view>
#include <vector>

namespace {

void
test_macros()
{
  static_assert(ARGS_SIZE() == 0);
  static_assert(ARGS_SIZE(1) == 1);
  static_assert(ARGS_SIZE(1, 2) == 2);
  static_assert(ARGS_SIZE(1, 2, 3) == 3);
  static_assert(ARGS_SIZE(1, 2, 3, 4) == 4);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5) == 5);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6) == 6);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7) == 7);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8) == 8);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9) == 9);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 10);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11) == 11);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12) == 12);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13) == 13);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14) == 14);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15) == 15);
  static_assert(ARGS_SIZE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) == 16);

  static_assert(ARGS_ELEMENT(0, 1) == 1);
  static_assert(ARGS_ELEMENT(1, 1, 2) == 2);
  static_assert(ARGS_ELEMENT(2, 1, 2, 3) == 3);
  static_assert(ARGS_ELEMENT(3, 1, 2, 3, 4) == 4);
  static_assert(ARGS_ELEMENT(4, 1, 2, 3, 4, 5) == 5);
  static_assert(ARGS_ELEMENT(5, 1, 2, 3, 4, 5, 6) == 6);
  static_assert(ARGS_ELEMENT(6, 1, 2, 3, 4, 5, 6, 7) == 7);
  static_assert(ARGS_ELEMENT(7, 1, 2, 3, 4, 5, 6, 7, 8) == 8);
  static_assert(ARGS_ELEMENT(8, 1, 2, 3, 4, 5, 6, 7, 8, 9) == 9);
  static_assert(ARGS_ELEMENT(9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 10);
  static_assert(ARGS_ELEMENT(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11) == 11);
  static_assert(ARGS_ELEMENT(11, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12) == 12);
  static_assert(ARGS_ELEMENT(12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13) == 13);
  static_assert(ARGS_ELEMENT(13, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14) == 14);
  static_assert(ARGS_ELEMENT(14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15) == 15);
  static_assert(ARGS_ELEMENT(15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) == 16);

  static_assert(TUPLE_SIZE(()) == 0);
  static_assert(TUPLE_SIZE((1)) == 1);
  static_assert(TUPLE_SIZE((1, 2)) == 2);
  static_assert(TUPLE_SIZE((1, 2, 3)) == 3);
  static_assert(TUPLE_SIZE((1, 2, 3, 4)) == 4);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5)) == 5);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6)) == 6);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7)) == 7);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8)) == 8);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9)) == 9);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10)) == 10);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)) == 11);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)) == 12);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)) == 13);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)) == 14);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)) == 15);
  static_assert(TUPLE_SIZE((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)) == 16);

  static_assert(TUPLE_ELEMENT(0, (1)) == 1);
  static_assert(TUPLE_ELEMENT(1, (1, 2)) == 2);
  static_assert(TUPLE_ELEMENT(2, (1, 2, 3)) == 3);
  static_assert(TUPLE_ELEMENT(3, (1, 2, 3, 4)) == 4);
  static_assert(TUPLE_ELEMENT(4, (1, 2, 3, 4, 5)) == 5);
  static_assert(TUPLE_ELEMENT(5, (1, 2, 3, 4, 5, 6)) == 6);
  static_assert(TUPLE_ELEMENT(6, (1, 2, 3, 4, 5, 6, 7)) == 7);
  static_assert(TUPLE_ELEMENT(7, (1, 2, 3, 4, 5, 6, 7, 8)) == 8);
  static_assert(TUPLE_ELEMENT(8, (1, 2, 3, 4, 5, 6, 7, 8, 9)) == 9);
  static_assert(TUPLE_ELEMENT(9, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)) == 10);
  static_assert(TUPLE_ELEMENT(10, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)) == 11);
  static_assert(TUPLE_ELEMENT(11, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)) == 12);
  static_assert(TUPLE_ELEMENT(12, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)) == 13);
  static_assert(TUPLE_ELEMENT(13, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)) == 14);
  static_assert(TUPLE_ELEMENT(14, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)) == 15);
  static_assert(TUPLE_ELEMENT(15, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)) == 16);
}

}

struct S
{
  int i;
  int j;
} __attribute__((aligned(8)));

DESCRIBE_STRUCT(S, i, j)

auto
main() -> int
{
  test_macros();

  std::vector<std::string_view> names;
  std::vector<int>              values;

  auto s = S{ 1, 2 };

  foreach_member_name<S>([&](auto member_name) {
    names.push_back(member_name);
  });
  foreach_member_pointer<S>([&](auto member_pointer) {
    values.push_back(s.*member_pointer);
  });

  ASSERT_EQ(std::string_view{ "i" }, names[0]);
  ASSERT_EQ(std::string_view{ "j" }, names[1]);

  ASSERT_EQ(1, values[0]);
  ASSERT_EQ(2, values[1]);

  static_assert(std::string_view{ TOSTRING(ARGS_TO_TUPLE(a, b, c)) } == "(a,b,c)");
  static_assert(std::string_view{ TOSTRING(ARGS_ELEMENT(0, TUPLE_TO_ARGS((a, b, c)))) } == "a");
  static_assert(std::string_view{ TOSTRING(ARGS_ELEMENT(1, TUPLE_TO_ARGS((a, b, c)))) } == "b");
  static_assert(std::string_view{ TOSTRING(ARGS_ELEMENT(2, TUPLE_TO_ARGS((a, b, c)))) } == "c");
}
