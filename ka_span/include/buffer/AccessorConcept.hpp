#pragma once

#include <util/Arithmetic.hpp>

#include <concepts>
#include <filesystem>

/// context to raw memory
template<class A>
concept AccessorConcept =

  std::equality_comparable<A>

  and

  requires {
    std::is_default_constructible_v<A>;
    std::is_move_constructible_v<A>;
    std::is_move_assignable_v<A>;
    std::is_copy_constructible_v<A>;
    std::is_copy_assignable_v<A>;

    typename A::value_type;
    std::is_default_constructible_v<typename A::value_type>;
    std::is_move_constructible_v<typename A::value_type>;
    std::is_move_assignable_v<typename A::value_type>;
    std::is_copy_constructible_v<typename A::value_type>;
    std::is_copy_assignable_v<typename A::value_type>;
  }

  and

  requires(A const ca, void* b, void const* cb, size_t index, typename A::value_type v) {
    /* Element access */
    { ca.get(cb, index) } -> std::convertible_to<decltype(v)>;
    { ca.set(b, index, v) } -> std::same_as<void>;

    /* Observers */
    { ca.empty() } -> std::convertible_to<bool>;
    { ca.size() } -> std::convertible_to<size_t>;
    { ca.size_bytes() } -> std::convertible_to<size_t>;
  };

/// context to raw memory
template<class A>
concept IterableAccessorConcept =

  AccessorConcept<A>

  and

  requires(A const ca, void* b, void const* cb, size_t index, typename A::value_type v) {
    /* Iterators */
    { ca.begin(b) } -> std::random_access_iterator<>;
    { ca.begin(cb) } -> std::random_access_iterator<>;
    { ca.cbegin(cb) } -> std::random_access_iterator<>;
    { ca.end(b) } -> std::random_access_iterator<>;
    { ca.end(cb) } -> std::random_access_iterator<>;
    { ca.cend(cb) } -> std::random_access_iterator<>;
  };
