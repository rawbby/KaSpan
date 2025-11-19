#include <util/pp.hpp>

#include <string_view>

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

auto
main() -> int
{
  test_macros();
  static_assert(std::string_view{ TOSTRING(ARGS_TO_TUPLE(a, b, c)) } == "(a,b,c)");
  static_assert(std::string_view{ TOSTRING(ARGS_ELEMENT(0, TUPLE_TO_ARGS((a, b, c)))) } == "a");
  static_assert(std::string_view{ TOSTRING(ARGS_ELEMENT(1, TUPLE_TO_ARGS((a, b, c)))) } == "b");
  static_assert(std::string_view{ TOSTRING(ARGS_ELEMENT(2, TUPLE_TO_ARGS((a, b, c)))) } == "c");
}
