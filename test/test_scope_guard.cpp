#include <kaspan/debug/assert.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <stdexcept>
#include <type_traits>

using namespace kaspan;

namespace {

void
test_basic()
{
  bool called = false;
  {
    auto const guard = scope_guard([&] {
      called = true;
    });
    ASSERT(not called);
  }
  ASSERT(called);
}

void
test_exception()
{
  bool called = false;
  try {
    auto const guard = scope_guard([&] {
      called = true;
    });
    throw std::runtime_error("force unwind");
  } catch (...) { // NOLINT(*-empty-catch)
    // destructor must have run
  }
  ASSERT(called);
}

void
test_macro_multiple()
{
  bool called_a = false;
  bool called_b = false;
  {
    SCOPE_GUARD(called_a = true);
    SCOPE_GUARD(called_b = true);
    ASSERT(not(called_a or called_b));
  }
  ASSERT(called_a and called_b);
}

} // namespace

auto
main() -> int
{
  using Dummy = decltype([] {});
  static_assert(not std::is_copy_constructible_v<scope_guard<Dummy>>, "scope_guard should be non-copyable");
  static_assert(not std::is_copy_assignable_v<scope_guard<Dummy>>, "scope_guard should be non-copy-assignable");
  static_assert(not std::is_move_constructible_v<scope_guard<Dummy>>, "scope_guard should be non-movable");
  static_assert(not std::is_move_assignable_v<scope_guard<Dummy>>, "scope_guard should be non-move-assignable");
  static_assert(std::is_nothrow_destructible_v<scope_guard<Dummy>>, "Destructor should be noexcept");

  test_basic();
  test_exception();
  test_macro_multiple();

  return 0;
}
