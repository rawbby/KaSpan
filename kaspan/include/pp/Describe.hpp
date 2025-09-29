#pragma once

#include <pp/PP.hpp>

#include <functional>
#include <tuple>

#define DESCRIBE_IMPL_MEMBER(M) &S::M
#define DESCRIBE_STRUCT(STRUCT, ...)                                        \
  constexpr inline auto member_name_impl(STRUCT** /* handle */) noexcept    \
  {                                                                         \
    return std::make_tuple(ARGS_MAP(TOSTRING, __VA_ARGS__));                \
  }                                                                         \
  constexpr inline auto member_pointer_impl(STRUCT** /* handle */) noexcept \
  {                                                                         \
    using S = STRUCT;                                                       \
    return std::make_tuple(ARGS_MAP(DESCRIBE_IMPL_MEMBER, __VA_ARGS__));    \
  }

template<class S>
void
foreach_member_name(auto f)
{
  auto kernel = [&f](auto const&... it) {
    (f(it), ...);
  };
  std::apply(kernel, member_name_impl(static_cast<S**>(nullptr)));
}

template<class S>
void
foreach_member_pointer(auto f)
{
  auto kernel = [&f](auto const&... it) {
    (f(it), ...);
  };
  std::apply(kernel, member_pointer_impl(static_cast<S**>(nullptr)));
}

template<class S>
void
enumerate_member_name(auto f)
{
  auto kernel = [&f](auto const&... it) {
    size_t i = 0;
    (f(i++, it), ...);
  };
  std::apply(kernel, member_name_impl(static_cast<S**>(nullptr)));
}

template<class S>
void
enumerate_member_pointer(auto f)
{
  auto kernel = [&f](auto const&... it) {
    size_t i = 0;
    (f(i++, it), ...);
  };
  std::apply(kernel, member_pointer_impl(static_cast<S**>(nullptr)));
}

template<class S>
constexpr auto
member_count_impl() -> size_t
{
  return std::tuple_size_v<decltype(member_pointer_impl(static_cast<S**>(nullptr)))>;
}

template<class S>
constexpr inline auto member_count = member_count_impl<S>();

template<class S>
concept IsDescribed =
  requires { member_name_impl(static_cast<S**>(nullptr)); } and
  requires { member_pointer_impl(static_cast<S**>(nullptr)); };
