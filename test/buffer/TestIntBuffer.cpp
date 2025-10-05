#include <buffer/Buffer.hpp>
#include <test/Assert.hpp>

#include <cassert>

template<IntConcept Int>
void
test_kernel()
{
  using Buf = IntBuffer<Int>;

  // create(0)
  {
    auto res = Buf::create(0);
    ASSERT(res.has_value());
    auto b = std::move(res.value());
    ASSERT_EQ(b.size(), 0);
    ASSERT_EQ(b.data(), nullptr);
    ASSERT_EQ(b.bytes(), 0);
  }

  // create(N) and read/write
  {
    constexpr size_t N   = 10;
    auto             res = Buf::create(N);
    ASSERT(res.has_value());
    auto b = std::move(res.value());
    ASSERT_EQ(b.size(), N);
    ASSERT_NE(b.data(), nullptr);
    ASSERT_EQ(b.bytes(), N * sizeof(Int));

    auto ptr = static_cast<Int*>(b.data());
    for (size_t i = 0; i < N; ++i)
      ptr[i] = static_cast<Int>(i * 7 + 1);

    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(ptr[i], static_cast<Int>(i * 7 + 1));
  }

  // zeroes(N)
  {
    constexpr size_t N   = 5;
    auto             res = Buf::zeroes(N);
    ASSERT(res.has_value());
    auto b = std::move(res.value());
    ASSERT_EQ(b.size(), N);
    auto const ptr = static_cast<Int const*>(b.data());
    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(ptr[i], 0);
  }

  // get / set vs [] vs raw
  {
    constexpr size_t N = 127;
    constexpr size_t S = 63;

    auto res = Buf::create(N);
    ASSERT(res.has_value());
    auto b = std::move(res.value());

    ASSERT_EQ(b.size(), N);

    for (size_t i = 0; i < S; ++i)
      b.set(i, static_cast<Int>(i));
    for (size_t i = S; i < N; ++i)
      b[i] = static_cast<Int>(i);

    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(b.get(i), i);

    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(b[i], i);

    auto ptr = static_cast<Int*>(b.data());
    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(ptr[i], i);
  }

  // move semantics
  {
    constexpr size_t N   = 3;
    auto             res = Buf::create(N);
    ASSERT(res.has_value());
    auto b1       = std::move(res.value());
    auto orig_ptr = b1.data();
    auto p        = static_cast<Int*>(b1.data());
    p[0]          = 11;
    p[1]          = 22;
    p[2]          = 33;

    auto b2 = std::move(b1);
    ASSERT_EQ(b2.size(), N);
    ASSERT_EQ(b2.data(), orig_ptr);
    auto const q = static_cast<Int const*>(b2.data());
    ASSERT_EQ(q[0], 11);
    ASSERT_EQ(q[1], 22);
    ASSERT_EQ(q[2], 33);
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
