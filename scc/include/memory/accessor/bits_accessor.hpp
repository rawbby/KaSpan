#pragma once

#include <memory/accessor/bits_mixin.hpp>
#include <memory/buffer.hpp>
#include <util/arithmetic.hpp>

#include <bit>
#include <cstdlib>
#include <cstring>
#include <utility>

class BitsAccessor final : public BitsMixin<BitsAccessor>
{
public:
  explicit BitsAccessor(void* data)
    : data_(data)
  {
  }

  BitsAccessor()  = default;
  ~BitsAccessor() = default;

  BitsAccessor(BitsAccessor const& rhs) noexcept = default;
  BitsAccessor(BitsAccessor&& rhs) noexcept      = default;

  auto operator=(BitsAccessor const& rhs) noexcept -> BitsAccessor& = default;
  auto operator=(BitsAccessor&& rhs) noexcept -> BitsAccessor&      = default;

  static auto borrow(void*& memory, u64 count) noexcept -> BitsAccessor
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    return BitsAccessor{ ::borrow<u64>(memory, u64_count) };
  }

  static auto borrow_clean(void*& memory, u64 count) noexcept -> BitsAccessor
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    return BitsAccessor{ ::borrow_clean<u64>(memory, u64_count) };
  }

  static auto borrow_filled(void*& memory, u64 count) noexcept -> BitsAccessor
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    return BitsAccessor{ ::borrow_filled<u64>(memory, u64_count) };
  }

  static auto create(u64 count) noexcept -> std::pair<Buffer, BitsAccessor>
  {
    auto  buffer = make_buffer<u64>((count + static_cast<u64>(63)) >> 6);
    auto* memory = buffer.data();
    return { std::move(buffer), BitsAccessor{ memory } };
  }

  static auto create_clean(u64 count) noexcept -> std::pair<Buffer, BitsAccessor>
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    auto       buffer    = make_buffer<u64>(u64_count);
    auto*      memory    = buffer.data();
    return { std::move(buffer), BitsAccessor{ ::borrow_clean<u64>(memory, u64_count) } };
  }

  static auto create_filled(u64 count) noexcept -> std::pair<Buffer, BitsAccessor>
  {
    auto const u64_count = (count + static_cast<u64>(63)) >> 6;
    auto       buffer    = make_buffer<u64>(u64_count);
    auto*      memory    = buffer.data();
    return { std::move(buffer), BitsAccessor{ ::borrow_filled<u64>(memory, ~0, u64_count) } };
  }

  [[nodiscard]] auto data() -> u64*
  {
    return static_cast<u64*>(data_);
  }

  [[nodiscard]] auto data() const -> u64 const*
  {
    return static_cast<u64 const*>(data_);
  }

private:
  void* data_ = nullptr;
};
