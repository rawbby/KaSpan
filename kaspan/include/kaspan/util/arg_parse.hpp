#pragma once

#include <kaspan/util/arithmetic.hpp>

#include <cstring>
#include <string>

namespace kaspan {

inline auto
arg_select_flag(int argc, char** argv, char const* flag) -> bool
{
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      return true;
    }
  }
  return false;
}

inline auto
arg_select_str(int argc, char** argv, char const* flag, void (*usage)(int, char**)) -> char const*
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      return argv[i + 1];
    }
  }
  usage(argc, argv);
  std::exit(1);
}

inline auto
arg_select_optional_str(int argc, char** argv, char const* flag) -> char const*
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      return argv[i + 1];
    }
  }
  return nullptr;
}

inline auto
arg_select_default_str(int argc, char** argv, char const* flag, char const* default_value) -> char const*
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      return argv[i + 1];
    }
  }
  return default_value;
}

template<IntConcept Int>
auto
arg_select_int(int argc, char** argv, char const* flag, void*(usage)(int, char**)) -> Int
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      auto const value = std::stoll(argv[i + 1]);
      DEBUG_ASSERT_LE(value, std::numeric_limits<Int>::max());
      DEBUG_ASSERT_GE(value, std::numeric_limits<Int>::min());
      return static_cast<Int>(value);
    }
  }
  usage(argc, argv);
  std::exit(1);
}

template<IntConcept Int>
auto
arg_select_optional_int(int argc, char** argv, char const* flag) -> std::optional<Int>
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      auto const value = std::stoll(argv[i + 1]);
      DEBUG_ASSERT_LE(value, std::numeric_limits<Int>::max());
      DEBUG_ASSERT_GE(value, std::numeric_limits<Int>::min());
      return static_cast<Int>(value);
    }
  }
  return std::nullopt;
}

template<IntConcept Int>
auto
arg_select_default_int(int argc, char** argv, char const* flag, Int default_value) -> Int
{
  for (int i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      auto const value = std::stoll(argv[i + 1]);
      DEBUG_ASSERT_LE(value, std::numeric_limits<Int>::max());
      DEBUG_ASSERT_GE(value, std::numeric_limits<Int>::min());
      return static_cast<Int>(value);
    }
  }
  return default_value;
}

} // namespace kaspan
