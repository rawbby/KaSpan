#pragma once

#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/integral_cast.hpp>

#include <type_traits>

namespace kaspan {

#define TMP() CAT(tmp, __COUNTER__)

#define PACK_TYPE_ALIAS_KERNEL(X) using CAT(X, _t) = std::remove_cvref_t<decltype(X)>;
#define PACK_MEMBER_DECLARATION_KERNEL(X) CAT(X, _t)(X);
#define PACK_FORWARD_KERNEL(X) std::forward<decltype(X)>(X)
#define PACK(...)                                                                                                                                                                  \
  [&]() {                                                                                                                                                                          \
    ARGS_FOREACH(PACK_TYPE_ALIAS_KERNEL, __VA_ARGS__)                                                                                                                              \
    struct pack /* NOLINT(*-pro-type-member-init) */                                                                                                                               \
    {                                                                                                                                                                              \
      ARGS_FOREACH(PACK_MEMBER_DECLARATION_KERNEL, __VA_ARGS__)                                                                                                                    \
    };                                                                                                                                                                             \
    return pack{ ARGS_MAP(PACK_FORWARD_KERNEL, __VA_ARGS__) };                                                                                                                     \
  }()

} // namespace kaspan
