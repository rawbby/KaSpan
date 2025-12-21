#pragma once

#include <type_traits>
#include <util/pp.hpp>

#define TMP() CAT(tmp, __COUNTER__)

#define PACK_TYPE_ALIAS_KERNEL(X) using CAT(X, _t) = std::remove_cvref_t<decltype(X)>;
#define PACK_MEMBER_DECLARATION_KERNEL(X) CAT(X, _t) X;
#define PACK_FORWARD_KERNEL(X) static_cast<decltype(X)&&>(X)
#define PACK(...) [&]() {                                     \
  ARGS_FOREACH(PACK_TYPE_ALIAS_KERNEL, __VA_ARGS__)           \
  struct Pack /* NOLINT(*-pro-type-member-init) */            \
  {                                                           \
    ARGS_FOREACH(PACK_MEMBER_DECLARATION_KERNEL, __VA_ARGS__) \
  };                                                          \
  return Pack{ ARGS_MAP(PACK_FORWARD_KERNEL, __VA_ARGS__) };  \
}()
