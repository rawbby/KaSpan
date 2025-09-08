#pragma once

#include <util/Arithmetic.hpp>

#include <concepts>
#include <cstdlib>

/// owning moveable auto freeing memory without context
template<class B>
concept BufferConcept =

  std::equality_comparable<B>

  and

  requires {
    std::is_default_constructible_v<B>;
    std::is_move_constructible_v<B>;
    std::is_move_assignable_v<B>;
    not std::is_copy_constructible_v<B>;
    not std::is_copy_assignable_v<B>;
  }

  and

  requires(B b, B const cb) {
    { b.data() } -> std::convertible_to<void*>;
    { cb.data() } -> std::convertible_to<void const*>;
  };
