#include <../include/index.h>

namespace {
using namespace ka_span;
}

static_assert(index_from_num(static_cast<std::uint64_t>(0x100000000ULL)).index() == 3);
static_assert(index_from_num(static_cast<std::uint64_t>(0xffffffffULL)).index() == 2);
static_assert(index_from_num(static_cast<std::uint64_t>(0x10000ULL)).index() == 2);
static_assert(index_from_num(static_cast<std::uint64_t>(0xffffULL)).index() == 1);
static_assert(index_from_num(static_cast<std::uint64_t>(0x100ULL)).index() == 1);
static_assert(index_from_num(static_cast<std::uint64_t>(0xffULL)).index() == 0);

static_assert(dispatch_indices_from_nums(
  [](auto u64, auto u16) {
    constexpr bool testU64 = std::is_same_v<decltype(u64), std::uint64_t>;
    constexpr bool testU16 = std::is_same_v<decltype(u16), std::uint16_t>;
    const bool testValU64  = u64 == static_cast<std::uint64_t>(0x100000000ULL);
    const bool testValU16  = u16 == static_cast<std::uint16_t>(0xff00ULL);
    return testU64 and testU16 and testValU64 and testValU16;
  },
  static_cast<std::uint64_t>(0x100000000ULL),
  static_cast<std::uint64_t>(0xff00ULL)));

static_assert(dispatch_indices_from_nums(
  [](auto u8, auto u32) {
    constexpr bool testU8  = std::is_same_v<decltype(u8), std::uint8_t>;
    constexpr bool testU32 = std::is_same_v<decltype(u32), std::uint32_t>;
    const bool testValU8   = u8 == static_cast<std::uint8_t>(0x7fULL);
    const bool testValU32  = u32 == static_cast<std::uint32_t>(0x1ffffULL);
    return testU8 and testU32 and testValU8 and testValU32;
  },
  static_cast<std::uint64_t>(0x7fULL),
  static_cast<std::uint64_t>(0x1ffffULL)));

int main()
{
}
