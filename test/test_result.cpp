// ReSharper disable CppDFAConstantFunctionResult
// ReSharper disable CppDeclaratorNeverUsed
// ReSharper disable CppDFAUnreadVariable
// ReSharper disable CppDFAUnreachableCode

#include "debug/assert_true.hpp"
#include "debug/assert_eq.hpp"
#include "debug/assert_ne.hpp"
#include <type_traits>
#include <util/result.hpp>

#include <string>
#include <utility>

namespace {

constexpr auto
int_success(int v) -> result<int>
{
  return result<int>(v);
}
constexpr auto
int_failure(ErrorCode e) -> result<int>
{
  return result<int>(e);
}

constexpr auto
foo_success() -> result<int>
{
  return 5;
}
constexpr auto
foo_fail() -> result<int>
{
  return ErrorCode::IO_ERROR;
}

constexpr auto
make_string_success(std::string s) -> result<std::string>
{
  return result<std::string>(std::move(s));
}
constexpr auto
make_string_failure() -> result<std::string>
{
  return result<std::string>(ErrorCode::SERIALIZE_ERROR);
}

void
test_basic_construction_and_access()
{
  result<int> r1 = 42;
  ASSERT(r1.has_value());
  ASSERT_EQ(r1.value(), 42);

  result<int> r2 = ErrorCode::ALLOCATION_ERROR;
  ASSERT(not r2.has_value());
  ASSERT_EQ(r2.error(), ErrorCode::ALLOCATION_ERROR);

  // VoidResult tests
  constexpr VoidResult v_ok = VoidResult::success();
  ASSERT(v_ok.has_value());
  ASSERT_EQ(v_ok.error_or_ok(), ErrorCode::OK);

  constexpr VoidResult v_fail = VoidResult::failure(ErrorCode::IO_ERROR);
  ASSERT(not v_fail.has_value());
  ASSERT_EQ(v_fail.error_or_ok(), ErrorCode::IO_ERROR);
}

void
test_value_or_and_move_accessors()
{
  result<std::string> const a = std::string("hello");
  ASSERT_EQ(a.value_or("alt"), "hello");

  result<std::string> const b = ErrorCode::ERROR;
  ASSERT_EQ(b.value_or(std::string("alt")), "alt");

  result<std::string> c = std::string("move_me");
  // rvalue value()
  auto const moved = std::move(c).value();
  ASSERT_EQ(moved, "move_me");
}

void
test_map_and_and_then()
{
  // map: int -> twice
  result<int> const r1 = 3;
  auto        r2 = r1.map([](int x) { return x * 2; });
  ASSERT((std::is_same_v<decltype(r2)::value_type, int>));
  ASSERT(r2.has_value());
  ASSERT_EQ(r2.value(), 6);

  // map on failure preserves error
  result<int> const rf  = ErrorCode::DESERIALIZE_ERROR;
  auto        rf2 = rf.map([](int) { return 1; });
  ASSERT(not rf2.has_value());
  ASSERT_EQ(rf2.error(), ErrorCode::DESERIALIZE_ERROR);

  // and_then: consumer returns Result<std::string>
  result<int> const pos  = 7;
  auto        rstr = pos.and_then([](int x) -> result<std::string> {
    if (x > 0) { return std::string("pos");
}
    return ErrorCode::ASSUMPTION_ERROR;
  });
  ASSERT(rstr.has_value());
  ASSERT_EQ(rstr.value(), "pos");

  result<int> const neg     = ErrorCode::MEMORY_MAPPING_ERROR;
  auto        neg_res = neg.and_then([](int) -> result<std::string> { return std::string("unused"); });
  ASSERT(not neg_res.has_value());
  ASSERT_EQ(neg_res.error(), ErrorCode::MEMORY_MAPPING_ERROR);
}

void
test_equality()
{
  constexpr result<int> ok_a  = 10;
  constexpr result<int> ok_b  = 10;
  constexpr result<int> ok_c  = static_cast<int>(ErrorCode::ERROR);
  constexpr result<int> err_a = ErrorCode::IO_ERROR;
  constexpr result<int> err_b = ErrorCode::IO_ERROR;
  constexpr result<int> err_c = ErrorCode::ERROR;

  ASSERT_EQ(ok_a, ok_b);
  ASSERT_NE(ok_a, ok_c);

  ASSERT_EQ(err_a, err_b);
  ASSERT_NE(err_a, err_c);

  ASSERT_NE(ok_a, err_a);
  ASSERT_NE(ok_c, err_c);
}

void
test_result_try_macro_no_var()
{
  auto ok_fn   = []() -> result<int> { return 1; };
  auto fail_fn = []() -> result<int> { return ErrorCode::IO_ERROR; };

  auto wrapper_ok = [&]() -> ErrorCode {
    RESULT_TRY(ok_fn());
    return ErrorCode::OK;
  };
  auto wrapper_fail = [&]() -> ErrorCode {
    RESULT_TRY(fail_fn());
    return ErrorCode::OK;
  };

  ASSERT(wrapper_ok() == ErrorCode::OK);
  ASSERT(wrapper_fail() == ErrorCode::IO_ERROR);
}

void
test_result_try_macro_with_var()
{
  auto ok_fn   = []() -> result<int> { return 7; };
  auto fail_fn = []() -> result<int> { return ErrorCode::SERIALIZE_ERROR; };

  auto wrapper_ok = [&]() -> result<int> {
    RESULT_TRY(int const x, ok_fn());
    ASSERT_EQ(x, 7);
    return ErrorCode::OK;
  };

  auto wrapper_fail = [&]() -> result<int> {
    RESULT_TRY(int const x, fail_fn());
    return ErrorCode::OK;
  };

  ASSERT_EQ(wrapper_ok(), ErrorCode::OK);
  ASSERT_EQ(wrapper_fail(), ErrorCode::SERIALIZE_ERROR);
}

void
test_assert_try_macro()
{
  auto f_ok = []() -> ErrorCode {
    RESULT_ASSERT(1 + 1 == 2, OK);
    return ErrorCode::OK;
  };

  auto f_fail = []() -> ErrorCode {
    RESULT_ASSERT(2 + 2 == 5, IO_ERROR);
    return ErrorCode::OK;
  };

  ASSERT(f_ok() == ErrorCode::OK);
  ASSERT(f_fail() == ErrorCode::IO_ERROR);
}

} // namespace

auto
main() -> int
{
  static_assert(std::is_copy_constructible_v<result<int>>);
  static_assert(std::is_copy_assignable_v<result<int>>);
  static_assert(std::is_nothrow_destructible_v<result<int>>);

  test_basic_construction_and_access();
  test_value_or_and_move_accessors();
  test_map_and_and_then();
  test_equality();
  test_result_try_macro_no_var();
  test_result_try_macro_with_var();
  test_assert_try_macro();

  return 0;
}
