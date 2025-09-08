#include <buffer/Accessor.hpp>
#include <buffer/Buffer.hpp>
#include <buffer/View.hpp>
#include <test/Assert.hpp>

#include <cassert>

template<IntConcept Int>
void
test_kernel()
{
  // create(0)
  {
    auto access = Accessor<Int>{ 0 };
    auto result = Buffer::create(access);
    ASSERT(result.has_value());
    auto buffer = std::move(result.value());

    ASSERT_EQ(access.size(), 0);
    ASSERT_EQ(access.size_bytes(), 0);
    ASSERT_EQ(buffer.data(), nullptr);
    ASSERT_EQ(buffer, nullptr);
  }

  // create(N) and read/write
  {
    constexpr size_t N = 10;

    auto access = Accessor<Int>{ N };
    auto result = Buffer::create(access);
    ASSERT(result.has_value());
    auto buffer = std::move(result.value());

    ASSERT_EQ(access.size(), N);
    ASSERT_EQ(access.size_bytes(), N * sizeof(Int));
    ASSERT_NE(buffer.data(), nullptr);
    ASSERT_NE(buffer, nullptr);

    auto ptr = static_cast<Int*>(buffer.data());
    for (size_t i = 0; i < N; ++i)
      ptr[i] = static_cast<Int>(i * 7 + 1);

    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(ptr[i], static_cast<Int>(i * 7 + 1));
  }

  // zeroes(N)
  {
    constexpr size_t N = 5;

    auto access = Accessor<Int>{ N };
    auto result = Buffer::zeroes(access);
    ASSERT(result.has_value());
    auto buffer = std::move(result.value());

    auto const ptr = static_cast<Int const*>(buffer.data());
    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(ptr[i], 0);
  }

  // get / set vs [] vs raw
  {
    constexpr size_t N = 127;
    constexpr size_t S = 63;

    auto access = Accessor<Int>{ N };
    auto result = Buffer::create(access);
    ASSERT(result.has_value());
    auto buffer = std::move(result.value());

    ASSERT_EQ(access.size(), N);

    for (size_t i = 0; i < S; ++i)
      access.set(buffer, i, static_cast<Int>(i));

    auto const ptr = static_cast<Int*>(buffer.data());
    for (size_t i = S; i < N; ++i)
      ptr[i] = static_cast<Int>(i);

    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(access.get(buffer, i), i);

    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(ptr[i], i);
  }

  // move semantics
  {
    constexpr size_t N = 3;

    auto access = Accessor<Int>{ N };
    auto result = Buffer::create(access);
    ASSERT(result.has_value());

    auto buffer = std::move(result.value());
    auto ptr    = buffer.data();
    auto p      = static_cast<Int*>(ptr);

    p[0] = 11;
    p[1] = 22;
    p[2] = 33;

    auto moved_buffer = std::move(buffer);
    auto moved_ptr    = moved_buffer.data();
    auto moved_p      = static_cast<Int*>(moved_ptr);

    ASSERT_EQ(moved_ptr, ptr);
    ASSERT_EQ(moved_p, p);
    ASSERT_EQ(moved_p[0], 11);
    ASSERT_EQ(moved_p[1], 22);
    ASSERT_EQ(moved_p[2], 33);
  }
}

auto
main() -> int
{
  test_kernel<i8>();
  test_kernel<i16>();
  test_kernel<i32>();
  test_kernel<i64>();
  test_kernel<u8>();
  test_kernel<u16>();
  test_kernel<u32>();
  test_kernel<u64>();
}
